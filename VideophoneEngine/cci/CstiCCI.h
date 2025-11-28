/*!
* \file CstiCCI.h
* \brief See CstiCCI.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

//
// Includes
//
#include "IstiVideophoneEngine.h"
#include "cmPropertyNameDef.h"
#include <memory>
#include <cstdio>

#include "stiOS.h"              // OS Abstraction layer
#include "stiSVX.h"
#include "stiTools.h"
#include "OptString.h"
#include "CstiCoreRequest.h"
#include "CstiCoreResponse.h"
#include "CstiMessageRequest.h"
#include "CstiConferenceRequest.h"
#include "CstiRemoteLoggerRequest.h"
#include "CstiRemoteLoggerService.h"
#include "CstiVPService.h"
#include "stiProductCompatibility.h"
#include "IstiNetwork.h"
#include "IstiMessageViewer.h"
#include "CstiImageManager.h"
#include "CstiVRCLServer.h"
#include <set>
#include "CstiBlockListManager.h"
#include "CstiRingGroupManager.h"
#include "CstiMessageManager.h"
#include "CstiCall.h"
#include "CstiPhoneNumberValidator.h"
#include "CstiRegistrationManager.h"
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "ServiceOutageClient.h"

#ifdef stiLDAP_CONTACT_LIST
#include "CstiLDAPDirectoryContactManager.h"
#endif

#include "ContactManager.h"

#ifdef stiCALL_HISTORY_MANAGER
#include "CstiCallHistoryManager.h"
#endif

#ifdef stiTUNNEL_MANAGER
#include "CstiTunnelManager.h"
#endif

#include "DhvClient.h"

#include "UserAccountManager.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
struct SstiRecordedMessageInfo;
struct SstiPersonalGreetingPreferences;
class CstiCCI;
class CstiVRCLServer;
class CstiConferenceManager;
class CstiCoreService;
class CstiConferenceService;
class CstiMessageService;
class CstiStateNotifyService;
class CstiRemoteLoggerService;
struct SstiPhoneSetting;

//
// Globals
//
const int g_nPRIORITY_VRCL = 0;
const int g_nPRIORITY_UI = 1;

//
//  Class Declaration
//
/*!
* \brief Conference Control Interface (CCI).
*
* Contains many of the interface APIs needed for the application developer.
*/
class CstiCCI : public IstiVideophoneEngine, public CstiEventQueue
{
public:

	/*!
	* \brief IP public IP address detect result codes.
	*/
	enum EIPDetectResult
	{
		eIP_DETECT_SUCCESS,           //!< IP detect succeeded.
		eIP_DETECT_FAILURE,           //!< IP detect failed.
		eIP_DETECT_CANCELLED,         //!< The IP detection process was canceled.
	}; // end EIPDetectResult

	CstiCCI (
		EstiInterfaceMode eLocalInterfaceMode,	///< Initial mode the object is created with
		bool bReportDialMethodDetermination);	///< Inform the application when a dial method is determined

	~CstiCCI ();

	CstiCCI (const CstiCCI &other) = delete;
	CstiCCI (CstiCCI &&other) = delete;
	CstiCCI &operator= (const CstiCCI &other) = delete;
	CstiCCI &operator= (CstiCCI &&other) = delete;

	stiHResult Initialize (
		ProductType productType,
		const std::string &version,
		bool verifyAddress,
		PstiAppGenericCallback callback,
		size_t callbackParam);

	stiHResult initializeHTTP ();

	stiHResult initializeCoreService (
		ProductType productType,
		const std::string &uniqueID);

	stiHResult initializeStateNotifyService (
		const std::string &uniqueID);

	stiHResult initializeMessageService (
		const std::string &uniqueID);

	stiHResult initializeConferenceService (
		const std::string &uniqueID);

	stiHResult initializeCloudServices (
		ProductType productType,
		const std::string &uniqueID);

	stiHResult initializeLoggingService (
		ProductType productType,
		const std::string &version,
		const std::string &uniqueID);

	stiHResult initializeConferenceManager (
		ProductType productType,
		const std::string &version,
		bool verifyAddress);

	void AppNotifyCallBackSet (
		PstiAppGenericCallback fpCallBack,
		size_t CallbackParam) final;

	/*!
	* \brief Performs default handling of messages.
	*
	* \param n32Message The message to handle
	* \param Param Parameter data associated to the message
	*/
	stiHResult MessageCleanup (
		int32_t n32Message,
		size_t Param) final;

	stiHResult Startup () final;

	stiHResult Shutdown () final;

	stiHResult ConferenceManagerStartup ();

	stiHResult ConferenceManagerShutdown ();

	stiHResult NetworkStartup () final;

	stiHResult NetworkShutdown () final;

	stiHResult ServicesStartup () final;

	stiHResult ServicesShutdown () final;
	
	stiHResult DiagnosticsSend ();
	
	//
	// Functions called only by the Application
	//

	EstiSwitch AutoRejectGet () const final;
	stiHResult AutoRejectSet (
		EstiSwitch eReject) final;

	int MaxCallsGet () const final;
	stiHResult MaxCallsSet (
		int numMaxCalls) final;

	stiHResult CallDial (
		EstiDialMethod eDialMethod,
		const std::string &dialString,
		const OptString &callListName,
		const OptString &relayLanguage,
		EstiDialSource eDialSource,
		IstiCall **ppCall) final;

	stiHResult CallDial (
		EstiDialMethod dialMethod,
		const std::string& dialString,
		const OptString& callListName,
		const OptString& relayLanguage,
		EstiDialSource dialSource,
		bool enableEncryption,
		IstiCall** ppCall) final;

	stiHResult CallDial (
		const CstiItemId &ContactId,
		const CstiItemId &PhoneNumberId,
		EstiDialSource eDialSource,
		bool enableEncryption,
		IstiCall **ppCall) final;

	stiHResult CallDial (
		EstiDialMethod eDialMethod,
		std::string dialString,
		const OptString &callListName,
		const OptString &fromPhoneNumber,
		const OptString &fromNameOverride,
		const OptString &relayLanguage,
		bool bAlwaysAllow,
		EstiDialSource eDialSource,
		bool enableEncryption,
		IstiCall **ppCall,
		const std::vector<SstiSipHeader>& additionalHeaders = {});

	stiHResult SignMailSend (
		std::string dialString,
		EstiDialSource dialSource,
		IstiCall **ppCall) final;

	stiHResult SignMailSend (
		const CstiItemId &contactId,
		const CstiItemId &phoneNumberPublicId,
		EstiDialSource dialSource,
		IstiCall **ppCall) final;

	stiHResult PhoneNumberReformat(
		const std::string &oldPhoneNumber,
		std::string *newPhoneNumber) final;

	bool DialStringIsValid(
		const std::string &phoneNumber) final;

	bool PhoneNumberIsValid(
		const std::string &phoneNumber) final;

	stiHResult CallObjectRemove (IstiCall *call) final;
	void allCallObjectsRemove () final;
	unsigned int CallObjectsCountGet ();
	unsigned int CallObjectsCountGet (uint32_t un32StateMask);

	stiHResult ConferencePortsSettingsSet (
		SstiConferencePortsSettings *pstConferencePorts) final;

	stiHResult ConferencePortsSettingsApply ();

	stiHResult ConferencePortsSettingsGet (
		SstiConferencePortsSettings *pstConferencePorts) const final;

	stiHResult ConferencePortsStatusGet (
		SstiConferencePortsStatus *pstConferencePorts) const final;

	stiHResult ConferencePortsSet (
		SstiConferencePorts *pstConferencePorts);

	stiHResult CoreLogin (
		const std::string &phoneNumber,
		const std::string &pin,
		const VPServiceRequestContextSharedPtr &context,
		unsigned int *punRequestId,
		const OptString &loginAs = nullptr) final;

	stiHResult CoreLogout (
		const VPServiceRequestContextSharedPtr &context,
		unsigned int *punRequestId) final;

	void CoreAuthenticationPinUpdate (
		const std::string &pin) final;

	stiHResult CoreRequestSend (
		CstiCoreRequest * poCoreRequest,
		unsigned int *punRequestId = nullptr) final;

	stiHResult CoreRequestSendEx (
		CstiCoreRequest * poCoreRequest,
		unsigned int *punRequestId) final;

	stiHResult ConferenceRequestSendEx (
		CstiConferenceRequest * poConferenceRequest,
		unsigned int *punRequestId = nullptr);

	stiHResult MessageRequestSend (
		CstiMessageRequest * poMessageRequest,
		unsigned int *punRequestId = nullptr) final;

	stiHResult MessageRequestSendEx (
		CstiMessageRequest * poMessageRequest,
		unsigned int *punRequestId = nullptr) final;

	void MessageRequestCancel (
		unsigned int unRequestId) final;

	stiHResult RemoteLogEventSend	(
		const std::string &buff) final;

	void StateNotifyHeartbeatRequest () final;
	void CurrentTimeSet (time_t ttCurrentTime);

	EstiInterfaceMode  LocalInterfaceModeGet () const final;
	stiHResult         LocalInterfaceModeSet (EstiInterfaceMode eMode) final;

	void DefaultProviderAgreementSet (bool bAgreed) final;

	stiHResult LocalAlternateNameGet (
		std::string *pAltName) final;

	stiHResult LocalNameSet (
		const std::string &name) final;

	stiHResult LocalNameGet (
		std::string *name) const final;	// A pointer to a character array to fill with the name.

	stiHResult LocalNamesSet ();

	stiHResult LocalNamesSet (
		const std::string &displayName,
		const std::string &alternateName);

	stiHResult LocalReturnCallInfoSet (
		EstiDialMethod eMethod,
		std::string dialString);

	stiHResult LocalReturnCallInfoUpdate ();

	stiHResult LocalNamesUpdate ();

	stiHResult UserPhoneNumbersSet (const SstiUserPhoneNumbers *psUserPhoneNumbers) final;
	stiHResult MaxSpeedsGet (uint32_t *pun32RecvSpeed, uint32_t *pun32SendSpeed) const final;
	stiHResult MaxSpeedsSet (uint32_t un32RecvSpeed, uint32_t un32SendSpeed) final;
	EstiAutoSpeedMode AutoSpeedSettingGet () const final;
	stiHResult MaxAutoSpeedsSet (uint32_t un32MaxAutoRecvSpeed, uint32_t un32MaxAutoSendSpeed) final;
	stiHResult MissedCallHandle (CstiCallSP call);
	stiHResult RingsBeforeGreetingGet(uint32_t *pun32CurrentMaxRings, uint32_t *pun32MaxRings) const final;
	stiHResult RingsBeforeGreetingSet(uint32_t un32RingsBeforeGreeting) final;
	stiHResult GreetingPreferencesGet(SstiPersonalGreetingPreferences *pGreetingPreferences) const final;
	stiHResult GreetingPreferencesSet(const SstiPersonalGreetingPreferences *pGreetingPreferences) final;
	stiHResult AudibleRingerMute (bool bMute) final;

	stiHResult PublicIPAddressGet (
		EstiPublicIPAssignMethod eAssignMethod,
		std::string *pAddress,
		EstiIpAddressType eIpAddressType) const final;

	stiHResult PublicIPAddressGet (
		std::string *pPublicIPAddress,
		EstiIpAddressType eIpAddressType);

	stiHResult PublicIPAddressInfoSet (
		EstiPublicIPAssignMethod eIPAssignMethod,
		const std::string &ipAddress) final;

	EstiPublicIPAssignMethod PublicIPAssignMethodGet () const final;

	void PrintScreen ();

	bool VCOUseGet () const final;
	stiHResult VCOUseSet (
		bool bUseVCO) final;
	EstiVcoType VCOTypeGet () const;
	stiHResult VCOTypeSet (
		EstiVcoType eVCOType) final;
	stiHResult AudioEnabledSet (
		bool eEnabled) final;
#ifdef stiTUNNEL_MANAGER
	bool TunnelingEnabledGet () const final;
	stiHResult TunnelingEnabledSet (
		bool eEnableTunneling) final;
#endif
	stiHResult VCOCallbackNumberGet (
		std::string *callbackNumber) const;
	stiHResult VCOCallbackNumberSet (
		const std::string &callbackNumber) final;

	//
	// Methods for recording greetings
	//
	stiHResult GreetingInfoGet() final;
	stiHResult GreetingUpload() final;

	//
	// Methods for viewing a message
	//
	void videoMessageGUIDRequest ();

	//
	// Methods for uploading a message
	//
	stiHResult RequestMessageUploadGUID ();

	//
	//::UNDOCUMENTED
	//

	//
	// Directory Services Callback functions
	//
	stiHResult DirectoryResolveReturn (const CstiCoreResponse* poResponse);

	//
	// Auto Detect Public IP Callback functions
	//
	stiHResult PublicIPResolved (const std::string &publicIp);

	stiHResult PublicIpChanged ();
	
	stiHResult AppNotify (
		int32_t n32Message,  	//!< Notification message ID.
		size_t MessageParam);	//!< Parameter to send.

#ifdef stiIMAGE_MANAGER
	IstiImageManager *ImageManagerGet () final;
#endif

	IstiBlockListManager *BlockListManagerGet () final;

	IstiRingGroupManager *RingGroupManagerGet () final;
	
#ifdef stiMESSAGE_MANAGER
	IstiMessageManager *MessageManagerGet () final;
#endif

	IstiContactManager *ContactManagerGet () final;
	
#ifdef stiLDAP_CONTACT_LIST
	IstiLDAPDirectoryContactManager *LDAPDirectoryContactManagerGet () final;
#endif

#ifdef stiCALL_HISTORY_MANAGER
	IstiCallHistoryManager *CallHistoryManagerGet () final;
#endif

#ifdef stiTUNNEL_MANAGER
	IstiTunnelManager* TunnelManagerGet () final;
#endif

	IUserAccountManager* userAccountManagerGet () final;

	stiHResult RelayLanguageListGet (
		std::vector <std::string> *pList) const final;
	
	void RelayLanguageSet (
		IstiCall *call,
		OptString langugage) final;

	stiHResult SipProxyAddressSet (const std::string &sipProxyAddress) final;

	bool GroupVideoChatAllowed (
			IstiCall *poCall1,
			IstiCall *poCall2) const final;
	stiHResult GroupVideoChatJoin (
			IstiCall *poCall1,
			IstiCall *poCall2) final;
	bool GroupVideoChatEnabled () const final;
	
	stiHResult dhviCreate (
			IstiCall *call) final;

	void AllowIncomingCallsModeSet (bool bAllow) final;

	std::string MessageStringGet (
		int32_t n32Message) final;
		
	EstiSecureCallMode SecureCallModeGet () final;
	
	void SecureCallModeSet (EstiSecureCallMode eSecureCallMode) final;
	
	DeviceLocationType deviceLocationTypeGet () final;
	
	void deviceLocationTypeSet (DeviceLocationType deviceLocationType) final;
	
	bool IPv6EnabledGet () final;
	stiHResult IPv6EnabledSet (bool bEnable) final;

	EstiHearingCallStatus HearingStatusGet() final;
	void HearingStatusSet (EstiHearingCallStatus eHearingStatus);

	std::shared_ptr<IstiContact> findContact(std::string dialString);
	std::shared_ptr<IstiContact> findContact(const CstiItemId &contactId);

	bool isOffline() final;

	bool isCustomerServiceNumber(std::string dialString) final;

	stiHResult portBackLogin(
		const std::string &phoneNumber,
		const std::string &password) final;
	
	void useProxyKeepAliveSet (bool useProxyKeepAlive) final;

	void externalCallSet(VRCLCallSP& call) final;

	stiHResult externalCallStatusSet(std::string& callStatus) final;

	stiHResult externalCallParticipantCountSet(int participantCount) final;

	ISignal<const SstiSharedContact&>& contactReceivedSignalGet () final;

	ISignal<const ServiceResponseSP<ClientAuthenticateResult>&>& clientAuthenticateSignalGet () final;

	ISignal<const ExternalConferenceData&>& externalConferenceSignalGet() final;

	ISignal<bool>& externalCameraUseSignalGet() final;
	
	// Since we've removed calls to Lock/Unlock calling this actually redirects
	// to CstiEventQueue::Lock(). Delete these methods unless explicit locking of the
	// event queue is needed.
	void Lock() = delete;
	void Unlock() = delete;

private:
	void conferenceManagerSignalsConnect ();
	void vrclSignalsConnect ();

//
// Private Members
//
private:

	bool m_bAudibleRingerMuted = false;

	PstiAppGenericCallback m_fpAppNotifyCB = nullptr;
	size_t m_CallbackParam = 0;

	// Indicates if the reconnect timer has been started
	EstiBool m_bDSReconnectStarted = estiFALSE;

	bool m_bTimeSet = false;

	bool m_bNeedsClockSync = false;		// Indicates that system clock is currently wrong, so delay any time-related actions

	std::string m_lastDialed;

	EstiBool m_bUpdating = estiFALSE;        // Flag indicating an update is in progress

	stiHResult VRCLPortSet (
		uint16_t un16Port) final;

	WillowPM::PropertyManager* m_poPM = nullptr;     // Pointer to the instance of the Property Manager Object

	unsigned int m_unDirectoryResolveRequestId = 0;
	unsigned int m_unPortingLogoutRequestId = 0;
	unsigned int m_unUserInterfaceGroupRequestId = 0;
	unsigned int m_unRegistrationInfoRequestId = 0;
	unsigned int m_unUserAccountInfoRequestId = 0;
	unsigned int m_unRingGroupInfoRequestId = 0;
	unsigned int m_unConferenceRoomCreateRequestId = 0;
	unsigned int m_dhviCreateRequestId = 0;

	std::recursive_mutex m_CallDialMutex;	// Used to keep a directory resolve from
									// using the call object before CallDial
									// is finished with it.
	std::recursive_mutex m_LeaveMessageMutex;
	bool m_bDeterminingDialMethod = false;	///< If true, we are trying to determine the dial method.
	bool m_bPossibleVCOCall = false;
	std::string m_Extension;
	bool m_bUseVCO = false;
	EstiVcoType m_ePreferredVCOType = stiVCO_PREFERRED_TYPE_DEFAULT;
	std::string m_vcoNumber;
	CstiCallSP m_pDirectoryResolveCall = nullptr;
	bool m_portingBack = false;

	std::recursive_mutex m_LockMutex;

	int m_nLocalNamePriority = g_nPRIORITY_UI;

	uint8_t m_un8t35Extension = 0;

	uint8_t m_un8t35CountryCode = 0;

	uint16_t m_un16ManufacturerCode = 0;

	ProductType m_productType = ProductType::Unknown;

	SstiUserPhoneNumbers m_sUserPhoneNumbers = {};

	EstiInterfaceMode m_eLocalInterfaceMode = {};

	std::string m_LastPublicIpSent;		// Contains the last Public IP address sent to core (empty string if it hasn't happened yet).

	int m_nCurrentSIPListenPort = nDEFAULT_SIP_LISTEN_PORT; // The port portion of the ip address

	SstiConferenceAddresses m_SIPConferenceAddresses = {};

	std::string m_LocalReturnDialString;
	EstiDialMethod m_eLocalReturnDialMethod = {};

	std::string m_DisplayName;
	std::string m_AlternateName;
	IstiMessageViewer *m_pFilePlay = nullptr;
	CstiConferenceManager *m_pConferenceManager = nullptr;
	CstiHTTPTask *m_pHTTPTask = nullptr;
	CstiHTTPTask *m_pHTTPLoggerTask = nullptr;
	CstiCoreService *m_pCoreService = nullptr;
	CstiMessageService *m_pMessageService = nullptr;
	CstiConferenceService *m_pConferenceService = nullptr;
	CstiStateNotifyService *m_pStateNotifyService = nullptr;
	CstiRemoteLoggerService *m_pRemoteLoggerService = nullptr;
	vpe::ServiceOutageClient m_serviceOutageClient;
#ifdef stiIMAGE_MANAGER
	CstiImageManager *m_pImageManager = nullptr;
#endif
	WillowPM::CstiBlockListManager *m_pBlockListManager = nullptr;
	CstiRingGroupManager *m_pRingGroupManager = nullptr;
#ifdef stiMESSAGE_MANAGER
	CstiMessageManager *m_pMessageManager = nullptr;
#endif
	CstiContactManager *m_pContactManager = nullptr;
#ifdef stiLDAP_CONTACT_LIST
	CstiLDAPDirectoryContactManager *m_pLDAPDirectoryContactManager = nullptr;
#endif
#ifdef stiCALL_HISTORY_MANAGER
	CstiCallHistoryManager *m_pCallHistoryManager = nullptr;
#endif

	CstiRegistrationManager * m_pRegistrationManager = nullptr;

	std::unique_ptr<UserAccountManager> m_userAccountManager;
	
	vpe::DhvClient m_dhvClient;

	unsigned int m_unGreetingInfoRequestId = 0;
	unsigned int m_unVideoMessageGUIDRequestId = 0;
	unsigned int m_unSignMailSendRequestId = 0;
	CstiCallSP m_spLeaveMessageCall = nullptr;
	bool m_vidPrivacyEnabled {false};
	vpe::EventTimer m_textOnlyGreetingTimer;
	vpe::EventTimer m_uploadGUIDRequestTimer;
#ifdef stiTHREAD_STATS
	vpe::EventTimer m_threadStatsTimer;
#endif
	vpe::EventTimer m_groupVideoChatRoomStatsTimer;
	vpe::EventTimer m_groupVideoChatRoomCreateTimer;
	vpe::EventTimer m_dhviRoomCreateTimer;
	
	CstiSignalConnection::Collection m_signalConnections;
	bool m_bSignMailGreetingLoaded = false;

	EstiHearingCallStatus m_eHearingStatus = {};
	
	CstiVRCLServer *m_pVRCLServer = nullptr;

	bool m_bServicesStarted = false; 			///< If the services have been started this is set to true.

	bool m_bStartEngine = false;				///< Indicates that the engine should be started when network connectivity is reached.

	bool m_bSignMailEnabled = false;

	// TextInSignMail variables
	std::string m_signMailText;
	int m_signMailTextStartSeconds = -1;
	int m_signMailTextEndSeconds = -1;

	std::string m_version;

	int m_numMaxCalls = 2;
	
	uint32_t m_un32MaxAutoRecvSpeed = stiMAX_AUTO_RECV_SPEED_DEFAULT;
	uint32_t m_un32MaxAutoSendSpeed = stiMAX_AUTO_SEND_SPEED_DEFAULT;

	timespec m_stCoreLoginStartTime = {};
	unsigned int m_unCoreLoginCount = 0;

	stiHResult ConferenceRoomStatusGet (
		CstiCallSP call);

	stiHResult FormatVRSDialString(
		const std::string &dialString,				//!< The dial string that is being dialed
		const OptString &preferredLanguage,
		std::string *serverDialString);				//!< A pointer to a buffer to which to write the results

	stiHResult LocalAlternateNameSet (
		const std::string &alternateName,
		int nPriority);

	//
	// Methods for recording a SignMail message
	//
	stiHResult CallInfoForSignMailSet(
		const CstiDirectoryResolveResult *poDirectoryResolveResult,
		CstiCallSP call);
	
	stiHResult LoadVideoGreeting(
		CstiCallSP call,
		EstiBool bPause = estiFALSE);

	stiHResult RecordMessageStart (
		CstiCallSP call);
	
	stiHResult SignMailStart (
		CstiCallSP call);
	
	stiHResult RecordMessageDelete (
		const SstiRecordedMessageInfo &recordedMsgInfo);
	stiHResult RecordMessageMailBoxFull (
		CstiCallSP call);
	stiHResult RecordMessageSend (
		const SstiRecordedMessageInfo &recordedMsgInfo);

	void UpdatePausePoint();

	void clientAuthenticateHandle (const ServiceResponseSP<ClientAuthenticateResult>& clientAuthenticateResult);
	void userAccountInfoGetHandle (const ServiceResponseSP<CstiUserAccountInfo>& userAccountInfoResponse);

	void userIdsSet (
		const std::string &userId,
		const std::string &groupId);

	//
	// Utility and Helper Functions
	//
	stiHResult VRSCallCreate (
		CstiCallSP call);
	
	stiHResult VrsCallCreate (
		EstiDialMethod eDialMethod,
		const CstiRoutingAddress &RoutingAddress,
		const std::string &dialString,
		const OptString &callListName,
		const OptString &relayLanguage,
		CstiCallSP *ppoCall);

	void vrsCallInfoUpdate (
		CstiCallSP call);

	stiHResult DirectoryResolveSubmit(
		const std::string &dialString,
		CstiCallSP *ppoCall,
		OptString fromPhoneNumber,
		const OptString &fromNameOverride,
		bool enableEncryption,
		const std::vector<SstiSipHeader>& additionalHeaders = {});

	stiHResult DialByDSPhoneNumber(
		const std::string &dialString,
		CstiCallSP *ppoCall,
		const OptString &fromPhoneNumber,
		const OptString &fromNameOverride,
		bool enableEncryption,
		const std::vector<SstiSipHeader>& additionalHeaders = {});

	stiHResult DialByDialString(
		const std::string &dialString,
		CstiCallSP *ppoCall,
		const OptString &fromPhoneNumber,
		const OptString &fromNameOverride,
		const OptString &callListName);

	stiHResult DialCommon (
		EstiDialMethod eDialMethod,
		const std::string &dialString,
		OptString callListName,
		const OptString &fromPhoneNumber,
		const OptString &fromNameOverride,
		OptString relayLanguage,
		bool bAlwaysAllow,
		bool enableEncryption,
		CstiCallSP *ppCall,
		const std::vector<SstiSipHeader>& additionalHeaders = {});

	void IncomingRingStart ();

	stiHResult UriListSend ();
	stiHResult PhoneOnlineSend ();
	stiHResult Login(
		const char *pszPhoneNumber,
		const char *pszPin,
		const VPServiceRequestContextSharedPtr &context,
		unsigned int *punRequestId,
		const char *pszLoginAs);

	static stiHResult CoreRequestAsyncCallback (
		int32_t n32Message,
		size_t MessageParam,
		size_t CallbackParam,
		size_t CallbackFromId);

	static stiHResult ThreadCallbackFilePlay(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);
	
	static stiHResult ThreadCallbackRingGroup(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);
	
	// Callback function for calls to CCI from child threads.
	static stiHResult ThreadCallback(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	struct InitializationSetting
	{
		InitializationSetting (
			const std::string &name,
			bool sendDefault)
		:
			name(name),
			sendDefault(sendDefault)
		{
		}

		std::string name;
		bool sendDefault = false;
	};

	stiHResult PropertiesCheck (
		const std::vector<InitializationSetting> &settings,
		int nSettingsType,
		bool *pbRequestSent);

	void PropertiesRemove (
		const std::vector<InitializationSetting> &settings);

	void SettingsGetProcess (
		CstiCoreResponse *poResponse);

	stiHResult UpdatePublicIP (
		const SstiNetworkSettings *pSettings);

	CstiCallSP FindCallByRequestId(
		unsigned int unRequestId); //!< The request id associated to the call object to find

	stiHResult DirectoryResolveRemoved(
		CstiVPServiceResponse *response);

	bool m_bRequestedPhoneSettings = false;

	stiHResult LocalInterfaceModeUpdate (
		EstiInterfaceMode eMode);

	stiHResult MaxSpeedsUpdate ();
	
	stiHResult AutoSpeedSettingsUpdate (
		uint32_t nAutoSpeedEnabled,
		EstiAutoSpeedMode eAutoSpeedMode,
		EstiAutoSpeedMode *eAutoSpeedSetting);
	
	stiHResult AutoSpeedMinStartSpeedUpdate (
		uint32_t un32AutoSpeedMinStartSpeed);

	void MaxCallsUpdate ();

	EstiSwitch BlockCallerIDEnabledGet () const final;
	EstiSwitch BlockCallerIDGet () const final;
	stiHResult BlockCallerIDSet (
		 EstiSwitch eBlock) final;

	EstiSwitch BlockAnonymousCallersGet () const final;
	stiHResult BlockAnonymousCallersSet (
		 EstiSwitch eBlock) final;

	//
	// Methods for handling property changes
	//
	stiHResult PublicIPSettingsChanged ();
	stiHResult HeartbeatDelayChanged ();
	stiHResult CoreServiceUrlChanged ();
	stiHResult StateNotifyServiceUrlChanged ();
	stiHResult MessageServiceUrlChanged ();
	stiHResult ConferenceServiceUrlChanged ();
	stiHResult RemoteLoggerServiceUrlChanged ();
	stiHResult CoreServiceSSLFailOverChanged ();
	stiHResult StateNotifySSLFailOverChanged ();
	stiHResult MessageServiceSSLFailOverChanged ();
	stiHResult ConferenceServiceSSLFailOverChanged ();
	stiHResult RemoteLoggerServiceSSLFailOverChanged ();
	stiHResult ConferencePortsSettingsChanged ();
	stiHResult CheckForAuthorizedNumberChanged ();
	stiHResult MaxSpeedsChanged ();
	stiHResult AutoSpeedSettingsChanged ();
	stiHResult AutoSpeedMinStartSpeedChanged ();
	stiHResult SignMailChanged ();
	stiHResult DTMFEnabledChanged ();
	stiHResult RealTimeTextEnabledChanged ();
	stiHResult BlockListEnabledChanged ();
	stiHResult RingsBeforeGreetingChanged ();
	stiHResult NATTraversalSIPChanged ();
#if 0
	stiHResult SIPTransportChanged ();
#endif
	stiHResult SIPNatTransportChanged ();
	stiHResult BlockCallerIDEnabledChanged ();
	stiHResult BlockCallerIDChanged ();
	stiHResult BlockAnonymousCallersChanged ();
	stiHResult IPv6EnabledChanged ();
	stiHResult LDAPChanged ();
	stiHResult SecureCallModeChanged ();
	stiHResult RingGroupSettingsChanged();
	stiHResult RemoteLoggerTraceLoggingChanged();
	stiHResult RemoteLoggerAssertLoggingChanged();
	stiHResult RemoteLoggerCallStatsLoggingChanged();
	stiHResult RemoteLoggerEventLoggingChanged();
	stiHResult RemoteFlowControlLoggingChanged();
	stiHResult ContactImagesEnableChanged ();
	stiHResult NewCallEnabledChanged ();
	stiHResult SipRestartTimeChanged ();
	stiHResult FirFeedbackEnabledChanged ();
	stiHResult PliFeedbackEnabledChanged ();
	stiHResult AfbFeedbackEnabledChanged ();
	stiHResult TmmbrFeedbackEnabledChanged ();
	stiHResult NackRtxFeedbackEnabledChanged ();
	stiHResult VRSFailOverTimeoutChanged ();
	stiHResult VRSFailoverDomainResolve ();
	stiHResult ServiceOutageServiceUrlChanged ();
	stiHResult ServiceOutageTimerIntervalChanged ();
	stiHResult signMailUpdateTimeChanged ();
	stiHResult videoUpdateTimeChanged ();
	stiHResult vrsFocusedRoutingChanged ();
	stiHResult dhvApiUrlChanged ();
	stiHResult dhvApiKeyChanged ();
	stiHResult dhvEnabledChanged ();
	stiHResult endpointMonitorServerChanged ();
	stiHResult endpointMonitorEnabledChanged ();
	stiHResult deviceLocationTypeChanged ();
	stiHResult deviceLocationPromptSecondsChanged ();
	stiHResult interfaceModeChanged ();

	static stiHResult ThreadCallbackCoreService(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	static stiHResult ThreadCallbackStateNotify(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	static stiHResult ThreadCallbackMessageService(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	static stiHResult ThreadCallbackConferenceService(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	static stiHResult ThreadCallbackRemoteLoggerService(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	static stiHResult ThreadCallbackHTTP(
		int32_t n32Message, 	///< holds the type of message
		size_t MessageParam,	///< holds data specific to the message
		size_t CallbackParam,	///< points to the instantiated CCI object
		size_t CallbackFromId);

	//
	// Registered Event handler methods
	//
	void stiEVENTCALL EventCoreServiceMessage (
		int32_t n32Message,
		size_t MessageParam);

	void stiEVENTCALL EventConferenceServiceMessage (
		int32_t n32Message,
		size_t MessageParam);

	void stiEVENTCALL EventStateNotifyMessage (
		int32_t n32Message,
		size_t MessageParam);

	void stiEVENTCALL EventMessageServiceMessage (
		int32_t n32Message,
		size_t MessageParam);

	void stiEVENTCALL EventRemoteLoggerServiceMessage (
		int32_t n32Message,
		size_t MessageParam);

	void ServiceOutageMessageConnect (
			std::function <void(std::shared_ptr<vpe::ServiceOutageMessage>)> callback) final;

	void ServiceOutageForceCheck () final;
	
	void stiEVENTCALL EventHTTPMessage (
		int32_t n32Message,
		size_t MessageParam);

	void stiEVENTCALL EventUpdateMessage (
		int32_t n32Message,
		size_t MessageParam);

	void stiEVENTCALL EventThreadCallback (
		int32_t n32Message,
		size_t MessageParam);

	void stiEVENTCALL EventConferenceRoomStatsTimeout ();
	void stiEVENTCALL EventConferenceRoomCreateTimeout ();

	void stiEVENTCALL EventDhviRoomCreateTimeout ();
	
	void stiEVENTCALL EventRemoteRingCountReceived (
		int ringCount);

	void stiEVENTCALL EventLocalRingCountReceived (
		uint32_t ringCount);

	void stiEVENTCALL EventSipMessageConfAddressChanged (
		SstiConferenceAddresses addresses);

	void stiEVENTCALL EventSipRegistrationConfirmed (
		std::string proxyIpAddress);

	void stiEVENTCALL EventPublicIpResolved (
		std::string ipAddress);

	void stiEVENTCALL EventCurrentTimeSet (
		time_t currentTime);

	void stiEVENTCALL EventDirectoryResolve (
		CstiCallSP call);

	void stiEVENTCALL EventConferencingWithMcu (
		CstiCallSP call);

	void stiEVENTCALL EventConferencePortsSet (const SstiConferencePorts &ports);

	void stiEVENTCALL EventIncomingCall (
		CstiCallSP call);

	void eventNetworkSettingsChanged ();

	void eventNetworkStateChange (IstiNetwork::EstiState eState);

	void eventMessageUploadGUIDRequest();
	void eventRecordedMessageDelete(
		const SstiRecordedMessageInfo &messageInfo);
	void eventRecordedMessageSend(
		const SstiRecordedMessageInfo &messageInfo);
	void eventSignMailGreetingSkipped();
	void eventFilePlayerStateSet(
		IstiMessageViewer::EState state);

	typedef stiHResult (CstiCCI::*PstiPropertyUpdateMethod) ();

	struct PropertyChangedMethod
	{
		const char *pszPropertyName;
		PstiPropertyUpdateMethod pfnMethod;
	};

	std::vector<PropertyChangedMethod> m_propertyChangedMethods;

	class classcomp
	{
	public:
		bool operator() (
			const PstiPropertyUpdateMethod& lhs,
			const PstiPropertyUpdateMethod& rhs) const
		{
			return &lhs < &rhs;
		}
	};

	stiHResult PropertiesSet (
		const CstiVRCLServer::PropertyList &list);

	void CheckProperty (
		const char *pszName,
		SstiSettingItem::EstiSettingType eType,
		const char *pszValue,
		std::set<PstiPropertyUpdateMethod, classcomp> *pSet);

	void CallPropertyUpdateMethods (
		const std::set<PstiPropertyUpdateMethod, classcomp> *pMethods);

	std::vector <CstiCoreResponse::SRelayLanguage> m_RelayLanguageList;

	SstiConferencePortsSettings m_stConferencePortsSettings = {};
	SstiConferencePortsStatus m_stConferencePortsStatus = {};

	bool m_bReportDialMethodDetermination = false;

	bool m_bRemoteLoggingCallStatsEnabled = false;
	
	bool m_bIPv6Enabled = stiIPV6_ENABLED_DEFAULT;

	std::set <unsigned int> m_PendingCoreRequests;

	bool GroupVideoChatAllowed (
		CstiCallSP call1,
		CstiCallSP call2,
		bool *pbInsufficientResources) const;

	bool GroupVideoChatAllowed (
		CstiCallSP call,
		bool *pbInsufficientResources) const;

	void SendSSLExpirationDates ();
	
	void NetworkChangeNotify() final;
	void ClientReRegister() final;
	void SipStackRestart() final;

	stiHResult HTTPProxyServerSet (
		const std::string &httpProxyAddress,
		uint16_t httpProxyPort) final;

	stiHResult HTTPProxyServerGet (
		std::string *httpProxyAddress,
		uint16_t *httpProxyPort) const final;
	
	bool NewRelayCallAllowed ();
	
	stiHResult ConnectedCallHold();
	
	stiHResult SpawnNewCall (
		const std::string &dialString);

	stiHResult RegistrationConferenceParamsSet ();

	bool RingGroupMemberDescriptionGet (
		const char *pszDialString,
		std::string *pDescription);

	EstiBool IsDialStringInRingGroup(
		const char *pszDialString);
	
	void CallStatusSend (
		CstiCallSP call,
		CstiSipCallLegSP callLeg);

	void AddCallToCallHistory (
		CstiCallSP call);

	stiHResult TransferLog (
		CstiCallSP call);

	bool m_bNewCallEnabled = false;

	time_t m_EngineCreationTime = 0;
	
	int m_nStackRestartDelta = stiSTACK_RESTART_DELTA_DEFAULT;

	EstiConferencingReadyState m_eConferencingReadyState = estiConferencingStateNotReady;

	void ConferencingReadyStateSet (
		EstiConferencingReadyState eConferencingReadyState);

	EstiConferencingReadyState ConferencingReadyStateGet ();
	
	std::string m_vrsFailoverIP;

	void userAccountInfoProcess (
		const CstiUserAccountInfo *userAccountInfo);

	bool m_mustCallCIR = false;

	void propertyChangedMethodsInitialize ();

	const std::vector<InitializationSetting> m_PhoneSettings =
	{
		{NAT_TRAVERSAL_SIP, true},
		{SIP_NAT_TRANSPORT, true},
		{IPv6Enabled, true},
		{cmMAX_RECV_SPEED, true},
		{cmMAX_SEND_SPEED, true},
		{HEVC_SIGNMAIL_PLAYBACK_ENABLED, true},
		{AUTO_SPEED_MODE, true},
		{ENABLE_CALL_BRIDGE, true},
		{DEVICE_LOCATION_TYPE, true}
	};

	const std::vector<InitializationSetting> m_UserSettings =
	{
		{RING_GROUP_DISPLAY_MODE, true},
		{RING_GROUP_ENABLED, true},
		{FAVORITES_ENABLED, true},
		{BLOCK_CALLER_ID_ENABLED, true},
		{BLOCK_CALLER_ID, true},
		{BLOCK_ANONYMOUS_CALLERS, true},
		{GROUP_VIDEO_CHAT_ENABLED, true},
		{NEW_CALL_ENABLED, true},
	#ifdef stiINTERNET_SEARCH
		{INTERNET_SEARCH_ENABLED, true},
	#endif
		{CONTACT_IMAGES_ENABLED, true},
		{DTMF_ENABLED, true},
		{REAL_TIME_TEXT_ENABLED, true},
		{PERSONAL_GREETING_ENABLED, true},
		{CALL_TRANSFER_ENABLED, true},
		{AGREEMENTS_ENABLED, true},
		{AUTO_SPEED_ENABLED, true},
		{GREETING_PREFERENCE, true},
		{DIRECT_SIGNMAIL_ENABLED, true},
		{cmRINGS_BEFORE_GREETING, true},
		{LDAP_ENABLED, false},
		{LDAP_SERVER, false},
		{LDAP_DOMAIN_BASE, false},
	#ifdef stiLDAP_CONTACT_LIST
		{LDAP_TELEPHONE_NUMBER_FIELD, false},
		{LDAP_HOME_NUMBER_FIELD, false},
		{LDAP_MOBILE_NUMBER_FIELD, false},
	#endif
		{LDAP_SERVER_PORT, false},
		{LDAP_SERVER_USE_SSL, false},
		{LDAP_SERVER_REQUIRES_AUTHENTICATION, false},
		{LDAP_SCOPE, false},
	#ifdef stiINTERNET_SEARCH
		{INTERNET_SEARCH_ALLOWED, false},
	#endif
		{SECURE_CALL_MODE, false},
	};

	CstiSignal<const SstiSharedContact&> contactReceivedSignal;
	CstiSignal<const ServiceResponseSP<ClientAuthenticateResult>&> clientAuthenticateSignal;
	CstiSignal<const ServiceResponseSP<CstiUserAccountInfo>&> userAccountInfoReceivedSignal;
	CstiSignal<const ServiceResponseSP<vpe::Address>&> emergencyAddressReceivedSignal;
	CstiSignal<const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>&> emergencyAddressStatusReceivedSignal;
	CstiSignal<EstiSecureCallMode> secureCallModeChangedSignal;
	CstiSignal<DeviceLocationType> deviceLocationTypeChangedSignal;
	CstiSignal<const ExternalConferenceData&> externalConferenceSignal;
	CstiSignal<bool> externalCameraUseSignal;

	stiHResult logInUsingCache (const std::string& phoneNumber, const std::string& pin) final;

	ISignal<const ServiceResponseSP<CstiUserAccountInfo>&>& userAccountInfoReceivedSignalGet () final;

	ISignal<const ServiceResponseSP<vpe::Address>&>& emergencyAddressReceivedSignalGet () final;

	ISignal<const ServiceResponseSP<EstiEmergencyAddressProvisionStatus>&>& emergencyAddressStatusReceivedSignalGet () final;

	ISignal<EstiSecureCallMode>& secureCallModeChangedSignalGet () final;
	
	ISignal<DeviceLocationType>& deviceLocationTypeChangedSignalGet () final;
	
	bool m_disallowOutgoingCalls { false };
};

//::DOCUMENTED
//:-----------------------------------------------------------------------------
//: Application Functions (inline)
//:-----------------------------------------------------------------------------
