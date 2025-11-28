/*!
* \file CstiVRCLServer.h
* \brief See CstiVRCLServer.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include "CstiEventQueue.h"
#include "CstiXMLCircularBuffer.h"
#include "IstiCall.h"
#include "IVRCLCall.h"
#include <list>
#include <string>
#include <sstream>
#include "stiClientValidate.h"
#include "ixml.h"
#include "stiOS.h"
#include "stiSVX.h"
#include "ExternalConferenceData.h"

//
// Constants
//
const int nMAX_MESSAGE_BUFFER = 484;

extern const char g_szUNKNOWN[];

struct SstiMessageDeleteInfo
{
	std::string CategoryName;
	int nCount;
};

//
// Forward Declarations
//
struct SstiCallDialInfo;
class CstiConferenceManager;
namespace WillowPM
{
	class PropertyManager;
}




class CstiVRCLServer : public CstiEventQueue
{
public:

	CstiVRCLServer () = delete;

	CstiVRCLServer (
		CstiConferenceManager *pConferenceManager);

	CstiVRCLServer (const CstiVRCLServer &other) = delete;
	CstiVRCLServer (CstiVRCLServer &&other) = delete;
	CstiVRCLServer &operator= (const CstiVRCLServer &other) = delete;
	CstiVRCLServer &operator= (CstiVRCLServer &&other) = delete;

	~CstiVRCLServer () override;

	stiHResult Initialize (
		const std::string &version);
	
	enum UpdateStatus
	{
		estiUPDATE_CURRENT,
		estiUPDATE_LOADING,
		estiUPDATE_INSTALING,
		estiUPDATE_REBOOTING,
		estiUPDATE_ERROR,
		estiUPDATE_IN_CALL
	};
	
	enum EDebugToolSetResult
	{
		estiDEBUG_TRACE_FLAG_SET_SUCCESSFUL,
		estiDEBUG_TRACE_FLAG_NOT_FOUND,
		estiDEBUG_TRACE_TAG_NOT_FOUND,

	};

	enum ECallIdSetResult
	{
		estiVRS_CALL_ID_SET_SUCCESSFUL,
		estiVRS_CALL_ID_SET_FAIL,

	};

	struct SstiPropertyInfo
	{
		std::string Name;
		std::string Type;
		std::string Value;
	};
	
	typedef std::list<SstiPropertyInfo> PropertyList;
	
	struct SstiContactInfo
	{
		std::string Name;
		std::string Number1;
		std::string Number2;
		std::string Number3;
		std::string Avatar;
		int VcoSet;
		std::string Language;
		int LightRingPattern;
	};
	
	typedef std::list<SstiContactInfo> ContactAddList;

	struct SstiContactEditInfo
	{
		SstiContactInfo Contact;
		int Id{0};
	};
	
	typedef std::list<SstiContactEditInfo> ContactEditList;

	stiHResult Startup ();
	stiHResult Shutdown ();

	inline EstiBool IsConnectedCheck ();

	stiHResult PortSet (uint16_t un16Port);
	uint16_t PortGet ();

	EstiResult VRCLNotify (
		EstiCmMessage eMsg);

	EstiResult VRCLNotify (
		EstiCmMessage eMsg,
	    VRCLCallSP call);

	EstiResult VRCLNotify (
		EstiCmMessage eMsg,
	    size_t param);

	void callDialError ();

	EstiResult HearingVideoConnecting (
		const std::string &conferenceURI,
		const std::string &initiator,
		const std::string &profileId);


	EstiBool StateChangeNotify (VRCLCallSP call);
	stiHResult BridgeStateChange (VRCLCallSP call);
	EstiResult ErrorSend (const SstiErrorLog * pstErrorLog);

	void UpdateCheckResult(int32_t n32Message);

	void DebugToolSetResult(int32_t n32Message);

	void CallIdSetResult(int32_t n32Message);

	void HearingStatusResponse();

	void RelayNewCallReadyResponse();

	void HearingCallSpawn(const SstiSharedContact &contact);
	
	void remoteGeolocationChanged (const std::string& geolocation);

	EstiBool SignMailEnabledGet () const
	    {return m_bSignMailEnabled;}

	void LoggedInPhoneNumberSet (const char *pszLoggedInPhoneNumber);

	void externalCallSet(VRCLCallSP& externalCall);

	VRCLCallSP callObjectHeadGet();
	VRCLCallSP callObjectGet(uint32_t stateMask);

	stiHResult externalConferenceStateChanged(const std::string& state);
	stiHResult externalParticipantCountChanged(int32_t count);

public:
	CstiSignal<> rebootDeviceSignal;
	CstiSignal<> diagnosticsGetSignal;
	CstiSignal<const SstiCallDialInfo &> callDialSignal;
	CstiSignal<const ExternalConferenceData&> externalConferenceSignal;
	CstiSignal<bool> externalCameraUseSignal;
	CstiSignal<const std::string &> localNameSetSignal;
	CstiSignal<> connectionEstablishedSignal;
	CstiSignal<> connectionClosedSignal;
	CstiSignal<const PropertyList &> propertiesSetSignal;
	CstiSignal<> signMailHangupSignal;
	CstiSignal<const std::string &> vrsCallIdSetSignal;
	CstiSignal<EstiHearingCallStatus> hearingStatusChangedSignal;
	CstiSignal<bool> relayNewCallReadySignal;
	CstiSignal<uint16_t*> textMessageSentSignal;
	CstiSignal<const std::string &> terpNetModeSetSignal;
	CstiSignal<> updateCheckNeededSignal;
	CstiSignal<> vrsAnnounceClearSignal;
	CstiSignal<const std::string &> vrsAnnounceSetSignal;
	CstiSignal<const std::string &, int, int> textInSignMailSetSignal;
	CstiSignal<const std::string &> hearingVideoCapabilitySignal;

private:
	
	EstiResult CallDialedEventSend (VRCLCallSP call);
	EstiResult CallIncomingEventSend (VRCLCallSP call);
	EstiResult CallConnectedEventSend (VRCLCallSP call);
	EstiResult CallTerminatedEventSend (VRCLCallSP call);
	stiHResult TextBackspaceSend ();
	stiHResult TextMessageSend (IXML_Node *pRootNode);
	stiHResult TextMessageClear (IXML_Node *pRootNode);
	stiHResult TerpNetModeSet (IXML_Node *pRootNode);
	stiHResult HearingStatus (IXML_Node *pRootNode);
	stiHResult RelayNewCallReady (IXML_Node *pRootNode);
	stiHResult hearingVideoCapability (IXML_Node *pRootNode);

private:
	CstiConferenceManager *m_pConferenceManager = nullptr;
	uint16_t m_un16ListeningPort = 0;
	int m_nIPv4ListeningSocket = stiINVALID_SOCKET;
	int m_nIPv6ListeningSocket = stiINVALID_SOCKET;
	int m_nClientSocket = stiINVALID_SOCKET;
	uint16_t m_un16Port = 0;
	uint32_t m_un32ClientIPAddress = 0;

	vpe::EventTimer m_connectionTimer;
	vpe::EventTimer m_callInProgressTimer;
	vpe::EventTimer m_lifeCheckTimer;
	vpe::EventTimer m_messagePlayTimer;

	CstiSignalConnection::Collection m_signalConnections;

	unsigned int m_unMessagePlayTime = 0;
	char m_szLastServerKey[MAX_KEY]{};
	char m_szLastClientKey[nMAX_MESSAGE_BUFFER]{};
	int m_nClientAPIMajorVersion = 0;
	int m_nClientAPIMinorVersion = 0;
	int m_unPendingLifeCheckResponses = 0;
	std::string m_applicationVersion;
	EstiBool m_bAuthenticated = estiFALSE;
	char m_szHearingPhoneNumber[un8stiDIAL_STRING_LENGTH + 1]{};
	EstiBool m_bCallInProgress = estiFALSE;
	EstiBool m_bSignMailEnabled = estiFALSE;
	std::string	m_strLoggedInPhoneNumber;
	VRCLCallSP m_externalCall = nullptr;

	CstiXMLCircularBuffer m_oMessageBuffer = {};
	WillowPM::PropertyManager *m_poPM = nullptr;

	// Xml Document members
	IXML_Document *m_pxDoc = nullptr;
	IXML_Node *m_pRootNode = nullptr;

	PropertyList m_PropertyList;

	CstiSignalConnection m_audioOutputDtmfReceivedSignalConnection;


	EstiResult ApiVersionSend ();
	EstiResult ApplicationVersionSend ();
	stiHResult ApplicationCapabilitiesSend();
	EstiResult ClientAdd (int32_t nSocket);
	EstiResult ClientDisconnect ();
	EstiResult ClientMessageProcess (char * byBuffer, int nBytesReadFromPort);
	EstiResult ClientResponseSend (const char * szMessage);
	EstiResult CallIsTransferableResponseSend ();
	stiHResult ListenSocketClose ();
	stiHResult ListenSocketOpen ();
	stiHResult IpSocketOpen (int nFamily, int *socket);
	EstiResult SettingSend (char * szData);
	EstiResult SettingChange (
		char * szParms, 
		char * szData, 
		uint32_t un32DataSize);
	EstiResult ConnectedStateSend (VRCLCallSP call);
	EstiResult StateSend ();
	EstiResult StateSend (VRCLCallSP call);
	stiHResult InCallStatsSend (VRCLCallSP call);
	char* strXescape (
		const char* szData,
		char* szBuf,
		int* nLen);
	int strXlen (const char* szBuf);

	void UserInfoAdd (VRCLCallSP call, std::stringstream *pMessage);

	EstiBool ClientVersionCheck (
		int nMajor,
		int nMinor);

	stiHResult PhoneNumberDial (IXML_Node *pRootNode);
	stiHResult UpdateCheck (IXML_Node *pRootNode);
	stiHResult DebugToolSet (IXML_Node *pRootNode);
	stiHResult CallIdSet (IXML_Node *pRootNode);
	stiHResult AllowedAudioCodecs (IXML_Node *pRootNode);
	stiHResult textInSignMailSet(IXML_Node *rootNode);
	stiHResult externalConferenceDial(IXML_Node* pRootNode);
	stiHResult externalCameraUse(IXML_Node *pRootNode);

	//
	// Event handlers.
	//
	void eventCallInProgressTimer ();
	void eventCallDialed (VRCLCallSP call);
	void eventCallIncoming (VRCLCallSP call);
	void eventCallConnected (VRCLCallSP call);
	void eventCallTerminated (VRCLCallSP call);
	void eventCallDtmfReceived (EstiDTMFDigit digit);
	void eventChangePort ();
	void eventError (std::shared_ptr<SstiErrorLog> errorLog);
	void eventStateChange (VRCLCallSP call);
	void eventBridgeStateChange (VRCLCallSP call);
	void eventSignMailGreetingStarted ();
	void eventSignMailRecordingStarted ();
	void eventSignMailAvailable ();
	void eventUpdateCheckResult (int32_t status);
	void eventDebugToolSetResult (EDebugToolSetResult result);
	void eventCallIdSetResult (ECallIdSetResult result);
	void eventTextMessageCleared ();
	void eventHearingStatusResponse ();
	void eventRelayNewCallReadyResponse ();
	void eventHearingCallSpawn (const SstiSharedContact &contact);
	void eventReadClientData (int clientSocket);
	void eventAcceptNewConnection (int listenSocket);
	void eventRemoteGeolocationChanged (const std::string& geolocation);
	void eventHearingVideoCapable (bool hearingVideoCapable);
	void eventHearingVideoConnected ();
	void eventHearingVideoConnectionFailed ();
	void eventHearingVideoDisconnected (bool hearingDisconnected);
	void eventHearingVideoConnecting(
		const std::string &conferenceURI,
		const std::string &initiator,
		const std::string &profileId);
};


////////////////////////////////////////////////////////////////////////////////
//; CstiVRCLServer::IsConnectedCheck
//
// Description: Checks whether or not a client is connected to the server
//
// Abstract:
//
// Returns:
//	estiTRUE if connected, otherwise estiFALSE
//
EstiBool CstiVRCLServer::IsConnectedCheck ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::IsConnectedCheck);

	// If we are connected, then we will have a valid socket
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		return (estiTRUE);
	} // end if

	return (estiFALSE);
} // end CstiVRCLServer::IsConnectedCheck
