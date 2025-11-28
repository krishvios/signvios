////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	CstiVRCLClient
//
//	File Name:	CstiVRCLClient.h

//
//	Abstract:
//		See CstiVRCLClient.cpp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIVRCLCLIENT_H
#define CSTIVRCLCLIENT_H

//
// Includes
//
#include <string>
#ifndef WIN32
#include <csignal>
#endif
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
#include "CstiXMLCircularBuffer.h"
#include "ixml.h"

//
// Constants
//
#define nMAX_CONNECT_TIMEOUT 10  // seconds
#define nMAX_VRCL_CLIENT_NAME     10
#define nMAX_VRCL_CLIENT_ID  100

//
// Typedefs
//
// extern const char g_szUNKNOWN[];
enum EstiVrclMessage
{
	estiMSG_VRCL_CLIENT_CONNECTED = estiMSG_NEXT,
	estiMSG_VRCL_CLIENT_DISCONNECT,
	estiMSG_VRCL_DEVICE_TYPE,
	estiMSG_VRCL_CLIENT_RMT_IS_RINGING,
	estiMSG_VRCL_CLIENT_APPLICATION_VERSION,
	estiMSG_VRCL_CLIENT_CALL_BANDWIDTH_DETECTION_BEGIN_SUCCESS,
	estiMSG_VRCL_CLIENT_CALL_TRANSFERABLE,
	estiMSG_VRCL_CLIENT_CALL_TRANSFER_SUCCESS,
// 	estiMSG_VRCL_CLIENT_CALL_TRANSFER_FAILURE,
	estiMSG_VRCL_AUDIO_PRIVACY_SET_SUCCESS,
// 	estiMSG_VRCL_AUDIO_PRIVACY_SET_FAILURE,
	estiMSG_VRCL_ALLOWED_AUDIO_CODECS_SUCCESS,
	estiMSG_VRCL_ALLOWED_AUDIO_CODECS_FAIL,
	estiMSG_VRCL_LOCAL_NAME_SET_SUCCESS,
	estiMSG_VRCL_VIDEO_PRIVACY_SET_SUCCESS,
	estiMSG_VRCL_SETTING_SET_SUCCESS,
	estiMSG_VRCL_SETTING_GET_SUCCESS,
// 	estiMSG_VRCL_VIDEO_PRIVACY_SET_FAILURE,
	estiMSG_VRCL_VRS_HEARING_NUMBER_SET_SUCCESS,
	estiMSG_VRCL_CLIENT_CONNECT_FAIL,
	estiMSG_VRCL_CLIENT_CALL_DIALED,
	estiMSG_VRCL_CLIENT_CALL_TERMINATED,
	estiMSG_VRCL_CLIENT_STATUS_OUTGOING,
	estiMSG_VRCL_CLIENT_STATUS_INCOMING,

	estiMSG_VRCL_VIDEO_MESSAGE_LIST_RESULT,
	estiMSG_VRCL_VIDEO_MESSAGE_PLAY_RESULT,
	
	estiMSG_VRCL_VIDEO_MESSAGES_DELETE_RESULT,

	estiMSG_VRCL_SIGNMAIL_ENABLED,
	estiMSG_VRCL_SIGNMAIL_AVAILABLE,
	
	estiMSG_VRCL_SIGNMAIL_GREETING_STARTED,

	estiMSG_CHECK_UPDATE_RESULT,

	estiMSG_VRCL_HEARING_STATUS_RESPONSE,
	estiMSG_VRCL_RELAY_NEW_CALL_READY_RESPONSE,
	estiMSG_VRCL_HEARING_CALL_SPAWN,

	// Add new enums above this one
	estiMSG_VRCL_NONE,
	estiMSG_VRCL_ANY
};


//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiVRCLClient : public CstiOsTaskMQ
{
public:

	CstiVRCLClient (
		PstiObjectCallback pfnAppCallback,
		size_t CallbackParam,
		bool bXmlVerbose);

	~CstiVRCLClient () override;

	CstiVRCLClient (const CstiVRCLClient &other) = delete;
	CstiVRCLClient (CstiVRCLClient &&other) = delete;
	CstiVRCLClient &operator= (const CstiVRCLClient &other) = delete;
	CstiVRCLClient &operator= (CstiVRCLClient &&other) = delete;

	stiHResult Startup () override;

	stiHResult Answer ();

	stiHResult APIVersionGet (int *pnMajorVersion, int *pnMinorVersion)
	{
		*pnMajorVersion = m_nServerAPIMajorVersion;
		*pnMinorVersion = m_nServerAPIMinorVersion;
		return stiRESULT_SUCCESS;
	};
	
	stiHResult AnnounceClear ();
	stiHResult AnnounceSet (
		const char *pszAnnounceText);
	stiHResult ApplicationVersionGet ();
	stiHResult AudioPrivacySet (EstiSwitch eSwitch);
	stiHResult CallBandwidthDetectionBegin ();
	stiHResult Connect (const char *pszServerIP, uint16_t un16Port = 0);
	stiHResult DeviceTypeGet ();
	stiHResult Dial (const char *pszDialString);
	stiHResult CallBridgeDial (const char *pszDialString);
	stiHResult CallBridgeDisconnect ();
	stiHResult AllowedAudioCodecs (const char *pszCodecs);
	stiHResult DialEx (
		const char *pszCallId,
		const char *pszTerpId,
		const char *pszHearingNumber,
		const char *pszHearingName,
		const char *pszDestNumber,
		bool bAlwaysAllow);
	stiHResult DialExx (
		const char *pszDestNumber,
		bool bAlwaysAllow);
	stiHResult EnableSignMailRecording (int bEnabled);
	stiHResult Hangup ();
	stiHResult IsCallTransferable ();
// 	stiHResult IsAlive ();
	stiHResult LocalNameSet (const char *pszName);
	stiHResult LoggedInPhoneNumberGet ();
	stiHResult TextMessageSend (const char *pszText);
	stiHResult DtmfMessageSend (char c);
	stiHResult DiagnosticsGet ();
	stiHResult TextBackspaceSend ();
	stiHResult Reboot ();
	stiHResult Reject ();
	stiHResult SettingGet (const char *pszType, const char *pszName);
	stiHResult SettingSet (const char *pszType, const char *pszName, const char *pszValue);
	stiHResult SkipToSignMailRecord ();
	stiHResult StatusCheck ();
	stiHResult Transfer (const char *szDialString);
	stiHResult CallHoldSet(EstiSwitch eSwitch);
	stiHResult VideoPrivacySet (EstiSwitch eSwitch);
	stiHResult VRSDial (const char *pszDialString);
	stiHResult VRSHearingPhoneNumberSet (const char *szDialString);
	stiHResult VRSVCODial (const char *pszDialString);
	stiHResult textInSignMail (
		const std::string &signMailText,
		const std::string &startSeconds,
		const std::string &endSeconds);
	stiHResult callIdSet (
		const std::string &callId);
	stiHResult hearingVideoCapability (
		const std::string &hearingNumber);

	stiHResult TerpNetModeSet (const char *pszMode);

	stiHResult HearingStatus (const char *pszStatus);
	stiHResult RelayNewCallReady (const char *pszStatus);

	stiHResult UpdateCheck ();
	stiHResult DebugToolSet(const char *pszName, const char *pzsValue, const char *pzsPersist);

	// IsConnected allows the user to detect whether we have a healthy connection;
	int IsConnected()
	{
		bool bResult = false;
		
		Lock ();
		
		bResult = (m_nClientSocket != stiINVALID_SOCKET);
		
		Unlock ();
		
		return bResult;
	}

	// Robust callbacks calls the callback function even if the function fails
	// this changes the behavior for TestVRCL. But others may like it too.
	void RobustCallbacksSet(bool bSetOnOff) {m_bRobustCallbacks=bSetOnOff;};

	// Let the user change or set the client ID (PHONE#)
	void ClientIDSet(int nClientID);

	// Let the user set the verbose setting.
	void XmlVerboseSet(bool bVerbose)
	{
		m_bXmlVerbose = bVerbose;
	}

private:

	// Event identifiers
	// These enumerations need to start at estiEVENT_NEXT or greater to not collide
	// with enumerations defined by the base classes.
	enum EEventType
	{
		// Basic messages
		estiEVENT_ANSWER = estiEVENT_NEXT,
		estiEVENT_APPLICATION_VERSION_GET,
		estiEVENT_AUDIO_PRIVACY_SET,
		estiEVENT_CALL_BANDWIDTH_DETECTION_BEGIN,
		estiEVENT_CONNECT,
		estiEVENT_DEVICE_TYPE_GET,
		estiEVENT_DIAL,
		estiEVENT_DIAL_EX,
		estiEVENT_HANGUP,
		estiEVENT_IS_CALL_TRANSFERABLE,
		estiEVENT_LOCAL_NAME_SET,
		estiEVENT_REBOOT,
		estiEVENT_REJECT,
		estiEVENT_SETTING_GET,
		estiEVENT_SETTING_SET,
		estiEVENT_STATUS_CHECK,
		estiEVENT_TRANSFER,
		estiEVENT_VIDEO_PRIVACY_SET,
		estiEVENT_CALL_HOLD_SET,
		estiEVENT_VRS_DIAL,
		estiEVENT_VRS_HEARING_NUMBER_SET,
		estiEVENT_VRS_VCO_DIAL,
		estiEVENT_CALL_BRIDGE_DIAL,
		estiEVENT_CALL_BRIDGE_DISCONNECT,
		estiEVENT_ALLOWED_AUDIO_CODECS,
		estiEVENT_VIDEO_MESSAGE_LIST,
		estiEVENT_VIDEO_MESSAGE_PLAY,
		estiEVENT_VIDEO_MESSAGES_DELETE,
		estiEVENT_ENABLE_SIGN_MAIL_RECORDING,
		estiEVENT_SKIP_TO_SIGN_MAIL_RECORD,
		estiEVENT_LOGGED_IN_PHONE_NUMBER_GET,
		estiEVENT_TEXT_MESSAGE_SEND,
		estiEVENT_TEXT_BACKSPACE_SEND,
		estiEVENT_DTMF_MESSAGE_SEND,
		estiEVENT_UPDATE_CHECK,
		estiEVENT_DEBUG_TOOL_SET,
		estiEVENT_VRS_ANNOUNCE_CLEAR,
		estiEVENT_VRS_ANNOUNCE_SET,
		estiEVENT_TERP_NET_MODE_SET,
		estiEVENT_DIAGNOSTICS_GET,
		estiEVENT_HEARING_STATUS_CHANGED,
		estiEVENT_RELAY_NEW_CALL_READY,
		estiEVENT_TEXT_IN_SIGNMAIL,
		estiEVENT_CALL_ID_SET,
		estiEVENT_HEARING_VIDEO_CAPABILITY,
	};

	int m_nClientSocket;
	int Task () override;
	CstiXMLCircularBuffer m_oMessageBuffer;
	int m_nServerAPIMajorVersion;
	int m_nServerAPIMinorVersion;
	bool m_bXmlVerbose;
	bool m_bRobustCallbacks;
	char m_strClientID[nMAX_VRCL_CLIENT_NAME + 1]{};		// Used by TestVRCL and others

	// Xml Document members
	IXML_Document *m_pxDoc;
	IXML_Node *m_pRootNode;

	stiHResult AnnounceClearHandle (CstiEvent *poEvent);
	stiHResult AnnounceSetHandle (CstiEvent *poEvent);
	stiHResult AnswerHandle (CstiEvent *poEvent);
	stiHResult ApplicationVersionGetHandle (CstiEvent *poEvent);
	stiHResult AudioPrivacySetHandle (CstiEvent *poEvent);
	stiHResult CallBandwidthDetectionBeginHandle (CstiEvent *poEvent);
	stiHResult ConnectHandle (CstiEvent *poEvent);
	stiHResult DeviceTypeGetHandle (CstiEvent *poEvent);
	stiHResult DialHandle (CstiEvent *poEvent);
	stiHResult CallBridgeDialHandle (CstiEvent *poEvent);
	stiHResult CallBridgeDisconnectHandle (CstiEvent *poEvent);
	stiHResult AllowedAudioCodecsHandle (CstiEvent *poEvent);
	stiHResult DialExHandle (CstiEvent *poEvent);
	stiHResult EnableSignMailRecordingHandle (CstiEvent *poEvent);
	stiHResult HangupHandle (CstiEvent *poEvent);
// 	stiHResult IsAlive (CstiEvent *poEvent);
	stiHResult IsCallTransferableHandle (CstiEvent *poEvent);
	stiHResult LocalNameSetHandle (CstiEvent *poEvent);
	stiHResult LoggedInPhoneNumberGetHandle (CstiEvent *poEvent);
	stiHResult TextMessageSendHandle (CstiEvent *poEvent);
	stiHResult DtmfMessageSendHandle (CstiEvent *poEvent);
	stiHResult DiagnosticsGetHandle (CstiEvent *poEvent);
	stiHResult TextBackspaceSendHandle (CstiEvent *poEvent);
	stiHResult RebootHandle (CstiEvent *poEvent);
	stiHResult RejectHandle (CstiEvent *poEvent);
	stiHResult SettingGetHandle (CstiEvent *poEvent);
	stiHResult SettingSetHandle (CstiEvent *poEvent);
	stiHResult SkipToSignMailRecordHandle (CstiEvent *poEvent);
	stiHResult StatusCheckHandle (CstiEvent *poEvent);
	stiHResult TransferHandle (CstiEvent *poEvent);
	stiHResult VideoPrivacySetHandle (CstiEvent *poEvent);
	stiHResult CallHoldSetHandle(CstiEvent* poEvent);
	stiHResult VRSDialHandle (CstiEvent *poEvent);
	stiHResult VRSHearingPhoneNumberSetHandle (CstiEvent *poEvent);
	stiHResult VRSVCODialHandle (CstiEvent *poEvent);
	stiHResult UpdateCheckHandle(CstiEvent *poEvent);
	stiHResult DebugToolSetHandle(CstiEvent *poEvent);
	stiHResult TerpNetModeSetHandle(CstiEvent *poEvent);
	stiHResult HearingStatusHandle(CstiEvent *poEvent);
	stiHResult RelayNewCallReadyHandle(CstiEvent *poEvent);
	stiHResult textInSignMailHandle(CstiEvent *poEvent);
	stiHResult callIdSetHandle(CstiEvent *poEvent);
	stiHResult hearingVideoCapabilityHandle(CstiEvent *poEvent);

	stiHResult ShutdownHandle (CstiEvent *poEvent);

	int VrclConnect (const char *pszRemoteHost, uint16_t un16Port);
	stiHResult MessageProcess (char *byBuffer, int nBytesReadFromPort);
	void RemoveQuotes(std::string &str);

#ifndef WIN32
	struct sigaction m_OldSigHandler{};
	stiHResult InitSigPipeHandler ();
	void UninitSigPipeHandler ();
#endif

	stiHResult VrclWrite (
		const char *buf,
		unsigned long *punLength);
	
	stiHResult VrclRead (
		char *pReturnBuf,
		int nReturnBufSize,
		int *pnBytesRead);

	stiDECLARE_EVENT_MAP(CstiVRCLClient);
	stiDECLARE_EVENT_DO_NOW (CstiVideoRecord);
};

#endif // CSTIVRCLCLIENT_H
// end file CstiVRCLClient.h
