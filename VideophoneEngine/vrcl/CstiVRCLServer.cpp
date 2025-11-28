/*!
* \file CstiVRCLServer.cpp
* \brief This server allows a client applications to remotely control the
*        video phone, such as placing calls.  It also allows a client to
*        receive notifications about a call.
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#include "CstiVRCLServer.h"
#include "stiTaskInfo.h"
#include "nonstd/observer_ptr.h"
#include "stiVRCLCommon.h"
#include "stiTrace.h"
#include "stiClientValidate.h"
#include "stiOS.h"
#include "IVRCLCall.h"
#include "CstiConferenceManager.h"
#include "stiBase64.h"
#include "PropertyManager.h"
#include "cmPropertyNameDef.h"
#include "stiTools.h"
#include "IstiVideoInput.h"
#include "IstiAudioInput.h"
#include "IstiAudioOutput.h"
#include "IstiPlatform.h"
#include "stiRemoteLogEvent.h"

#if defined stiLINUX
#include <sys/socket.h>
#endif
#include <cctype> // for isdigit
#include <memory>
#include <sstream>

using namespace WillowPM;


//
// Constants
//
const int nMAX_CONNECTION_TIME = 3000; // 3 seconds (in milliseconds)
const int nCALL_IN_PROGRESS_INTERVAL = 5000; // 5 seconds (in milliseconds)
const int nLIFE_CHECK_INTERVAL = 10000; // 10 seconds (in milliseconds)

static const char g_szConnectMessage[] = "<Connect>";
static const char g_szValidateMessage[] = "<Validate>";
static const char g_szAddressDialMessage[] = "<AddressDial>";
static const char g_szCallBridgeDialMessage[] = "<CallBridgeDial>";
static const char g_szEnableCallBridge[] = "<EnableCallBridge />";
static const char g_szCallTransferMessage[] = "<CallTransfer>";
static const char g_szVRSDialMessage[] = "<VRSDial>";
static const char g_szVRSVCODialMessage[] = "<VRSVCODial>";
static const char g_szVRSHearingPhoneSetMessage[] = "<VRSHearingPhoneSet>";
static const char g_szLocalNameSetMessage[] = "<LocalNameSet>";
static const char g_szAPIVersionGetMessage[] = "<APIVersionGet>";
static const char g_szSettingGetMessage[] = "<SettingGet"; // Note: The end bracket is intentionally missing!
static const char g_szSettingSetMessage[] = "<SettingSet"; // Note: The end bracket is intentionally missing!
static const char g_szCallHoldSetMessage[] = "<CallHoldSet>";
static const char g_szVideoPrivacySetMessage[] = "<VideoPrivacySet>";
static const char g_szAudioPrivacySetMessage[] = "<AudioPrivacySet>";


#if 0
static const char g_szPrintScreenMessage[] = "<PrintScreen />";
#endif

static const char g_szConnectAck[] = "<ConnectAck />";
static const char g_szConnectAckBegin[] = "<ConnectAck>";
static const char g_szConnectAckEnd[] = "</ConnectAck>";
static const char g_szValidateAck[] = "<ValidateAck />";
static const char g_szCallBandwidthDetectionBeginSuccess[] = "<CallBandwidthDetectionBeginSuccess />";
static const char g_szCallBandwidthDetectionBeginFail[] = "<CallBandwidthDetectionBeginFail />";
static const char g_szCallConnectedEx[] = "<CallConnectedEx>";
static const char g_szCallConnectedExEnd[] = "</CallConnectedEx>";
static const char g_szAPIVersionMessage[] = "<APIVersion>";
static const char g_szAPIVersionMessageEnd[] = "</APIVersion>";
static const char g_szApplicationVersion[] = "<ApplicationVersion>";
static const char g_szApplicationVersionEnd[] = "</ApplicationVersion>";
static const char g_szNumberSetting[] = "<Setting Type =\"%s\" Name =\"%s\">%d</Setting>";
static const char g_szStringSetting[] = "<Setting Type =\"%s\" Name =\"%s\">%s</Setting>";
static const char g_szSettingGetFail[] = "<SettingGetFail />";
static const char g_szSettingSetSuccess[] = "<SettingSetSuccess />";
static const char g_szSettingSetFail[] = "<SettingSetFail />";
static const char g_szDeviceTypeMessage[] = "<DeviceType>";
static const char g_szDeviceTypeMessageEnd[] = "</DeviceType>";
static const char g_szVideoPrivacySetSuccess[] = "<VideoPrivacySetSuccess />";
static const char g_szVideoPrivacySetFail[] = "<VideoPrivacySetFail />";
static const char g_szCallHoldSetSuccess[] = "<CallHoldSetSuccess />";
static const char g_szCallHoldSetFail[] = "<CallHoldSetFail />";
static const char g_szHoldSetSuccess[] = "<HoldSetSuccess />";
static const char g_szHoldSetFail[] = "<HoldSetFail />";
static const char g_szAudioPrivacySetSuccess[] = "<AudioPrivacySetSuccess />";
static const char g_szAllowedAudioCodecsFail[] = "<AllowedAudioCodecsFail />";
static const char g_szAllowedAudioCodecsSuccess[] = "<AllowedAudioCodecsSuccess />";
static const char g_szAudioPrivacySetFail[] = "<AudioPrivacySetFail />";

static const char g_szName[] = "<Name>";
static const char g_szNameEnd[] = "</Name>";
static const char g_szMACAddress[] = "<MACAddress>";
static const char g_szMACAddressEnd[] = "</MACAddress>";
static const char g_szIPAddress[] = "<IPAddress>";
static const char g_szIPAddressEnd[] = "</IPAddress>";
static const char g_szPhoneNumber[] = "<PhoneNumber>";
static const char g_szPhoneNumberEnd[] = "</PhoneNumber>";
static const char g_szPhoneNumberLocal[] = "<PhoneNumberLocal>";
static const char g_szPhoneNumberLocalEnd[] = "</PhoneNumberLocal>";
static const char g_szPhoneNumberTollFree[] = "<PhoneNumberTollFree>";
static const char g_szPhoneNumberTollFreeEnd[] = "</PhoneNumberTollFree>";
static const char g_szPhoneNumberSorenson[] = "<PhoneNumberSorenson>";
static const char g_szPhoneNumberSorensonEnd[] = "</PhoneNumberSorenson>";
static const char g_szPhoneNumberRingGroupLocal[] = "<PhoneNumberRingGroupLocal>";
static const char g_szPhoneNumberRingGroupLocalEnd[] = "</PhoneNumberRingGroupLocal>";
static const char g_szPhoneNumberRingGroupTollFree[] = "<PhoneNumberRingGroupTollFree>";
static const char g_szPhoneNumberRingGroupTollFreeEnd[] = "</PhoneNumberRingGroupTollFree>";
static const char g_szUserId[] = "<UserId>";
static const char g_szUserIdEnd[] = "</UserId>";
static const char g_szGroupUserId[] = "<GroupUserId>";
static const char g_szGroupUserIdEnd[] = "</GroupUserId>";
static const char g_szProductName[] = "<ProductName>";
static const char g_szProductNameEnd[] = "</ProductName>";
static const char g_szProductVersion[] = "<ProductVersion>";
static const char g_szProductVersionEnd[] = "</ProductVersion>";
static const char g_szCallIsTransferable[] = "<CallIsTransferable />";
static const char g_szCallIsNotTransferable[] = "<CallIsNotTransferable />";
static const char g_szVCO[] = "<VCO>";
static const char g_szVCOEnd[] = "</VCO>";
static const char g_szProtocol[] = "<Protocol>";
static const char g_szProtocolEnd[] = "</Protocol>";
static const char g_szVerifyAddress[] = "<VerifyAddress>";
static const char g_szVerifyAddressEnd[] = "</VerifyAddress>";
static const char g_szRealTimeTextSupported[] = "<RealTimeTextSupported />";

// Return In Call Stats
static const char g_szInCallStats[] = "<InCallStats>";
static const char g_szInCallStatsEnd[] = "</InCallStats>";

static const char g_szCurrentCallStats[] = "<CurrentCallStats>";
static const char g_szCurrentCallStatsEnd[] = "</CurrentCallStats>";
static const char g_szDuration[] = "<Duration>";
static const char g_szDurationEnd[] = "</Duration>";
static const char g_szVideoCodecsSent[] = "<VideoCodecsSent>";
static const char g_szVideoCodecsSentEnd[] = "</VideoCodecsSent>";
static const char g_szVideoCodecsRecv[] = "<VideoCodecsRecv>";
static const char g_szVideoCodecsRecvEnd[] = "</VideoCodecsRecv>";
static const char g_szVideoFrameSizeSent[] = "<VideoFrameSizeSent>";
static const char g_szVideoFrameSizeSentEnd[] = "</VideoFrameSizeSent>";
static const char g_szVideoFrameSizeRecv[] = "<VideoFrameSizeRecv>";
static const char g_szVideoFrameSizeRecvEnd[] = "</VideoFrameSizeRecv>";
static const char g_szTargetFPS[] = "<TargetFPS>";
static const char g_szTargetFPSEnd[] = "</TargetFPS>";
static const char g_szActualFPSSent[] = "<ActualFPSSent>";
static const char g_szActualFPSSentEnd[] = "</ActualFPSSent>";
static const char g_szActualFPSRecv[] = "<ActualFPSRecv>";
static const char g_szActualFPSRecvEnd[] = "</ActualFPSRecv>";
static const char g_szTargetVideoKbps[] = "<TargetVideoKbps>";
static const char g_szTargetVideoKbpsEnd[] = "</TargetVideoKbps>";
static const char g_szActualVideoKbpsSent[] = "<ActualVideoKbpsSent>";
static const char g_szActualVideoKbpsSentEnd[] = "</ActualVideoKbpsSent>";
static const char g_szActualVideoKbpsRecv[] = "<ActualVideoKbpsRecv>";
static const char g_szActualVideoKbpsRecvEnd[] = "</ActualVideoKbpsRecv>";
static const char g_szReceivedPackeLossPercent[] = "<ReceivedPackeLossPercent>";
static const char g_szReceivedPackeLossPercentEnd[] = "</ReceivedPackeLossPercent>";

static const char g_szReceivedVideoStats[] = "<ReceivedVideoStats>";
static const char g_szReceivedVideoStatsEnd[] = "</ReceivedVideoStats>";
static const char g_szTotalPacketsReceived[] = "<TotalPacketsReceived>";
static const char g_szTotalPacketsReceivedEnd[] = "</TotalPacketsReceived>";
static const char g_szTotalFramesAssembledAndSentToDecoder[] = "<TotalFramesAssembledAndSentToDecoder>";
static const char g_szTotalFramesAssembledAndSentToDecoderEnd[] = "</TotalFramesAssembledAndSentToDecoder>";
static const char g_szLostPackets[] = "<LostPackets>";
static const char g_szLostPacketsEnd[] = "</LostPackets>";

static const char g_szVideoPlaybackFrameGetErrors[] = "<VideoPlaybackFrameGetErrors>";
static const char g_szVideoPlaybackFrameGetErrorsEnd[] = "</VideoPlaybackFrameGetErrors>";
static const char g_szKeyframesRequested[] = "<KeyframesRequested>";
static const char g_szKeyframesRequestedEnd[] = "</KeyframesRequested>";
static const char g_szKeyframesReceived[] = "<KeyframesReceived>";
static const char g_szKeyframesReceivedEnd[] = "</KeyframesReceived>";
static const char g_szKeyframeWaitTimeMax[] = "<KeyframeWaitTimeMax>";
static const char g_szKeyframeWaitTimeMaxEnd[] = "</KeyframeWaitTimeMax>";
static const char g_szKeyframeWaitTimeMin[] = "<KeyframeWaitTimeMin>";
static const char g_szKeyframeWaitTimeMinEnd[] = "</KeyframeWaitTimeMin>";
static const char g_szKeyframeWaitTimeAvg[] = "<KeyframeWaitTimeAvg>";
static const char g_szKeyframeWaitTimeAvgEnd[] = "</KeyframeWaitTimeAvg>";
static const char g_szKeyframeWaitTimeTotal[] = "<KeyframeWaitTimeTotal>";
static const char g_szKeyframeWaitTimeTotalEnd[] = "</KeyframeWaitTimeTotal>";
static const char g_szErrorConcealmentEvents[] = "<ErrorConcealmentEvents>";
static const char g_szErrorConcealmentEventsEnd[] = "</ErrorConcealmentEvents>";
static const char g_szIFramesDiscardedAsIncomplete[] = "<I-FramesDiscardedAsIncomplete>";
static const char g_szIFramesDiscardedAsIncompleteEnd[] = "</I-FramesDiscardedAsIncomplete>";
static const char g_szPFramesDiscardedAsIncomplete[] = "<P-FramesDiscardedAsIncomplete>";
static const char g_szPFramesDiscardedAsIncompleteEnd[] = "</P-FramesDiscardedAsIncomplete>";
static const char g_szGapsInFrameNumbers[] = "<GapsInFrameNumbers>";
static const char g_szGapsInFrameNumbersEnd[] = "</GapsInFrameNumbers>";

static const char g_szSentVideoStats[] = "<SentVideoStats>";
static const char g_szSentVideoStatsEnd[] = "</SentVideoStats>";
static const char g_szKeyframesSent[] = "<KeyframesSent>";
static const char g_szKeyframesSentEnd[] = "</KeyframesSent>";
static const char g_szFramesLostFromRCU[] = "<FramesLostFromRCU>";
static const char g_szFramesLostFromRCUEnd[] = "</FramesLostFromRCU>";
static const char g_szPartialFramesSent[] = "<PartialFramesSent>";
static const char g_szPartialFramesSentEnd[] = "</PartialFramesSent>";

static const char g_szCallDialed[] = "<CallDialed>";
static const char g_szCallDialedEnd[] = "</CallDialed>";
static const char g_szCallBridgeFail[] = "<CallBridgeFail>";
static const char g_szCallBridgeFailEnd[] = "</CallBridgeFail>";
static const char g_szCallBridgeSuccess[] = "<CallBridgeSuccess />";
static const char g_szCallBridgeDisconnectFail[] = "<CallBridgeDisconnectFail />";
static const char g_szCallBridgeDisconnectSuccess[] = "<CallBridgeDisconnectSuccess />";
static const char g_szCallBridgeEnabled[] = "<CallBridgeEnabled />";
static const char g_szCallBridgeDisabled[] = "<CallBridgeDisabled />";
static const char g_szCallIncoming[] = "<CallIncoming>";
static const char g_szCallIncomingEnd[] = "</CallIncoming>";
static const char g_szCallTerminated[] = "<CallTerminated>";
static const char g_szCallTerminatedEnd[] = "</CallTerminated>";
static const char g_szCallDtmfReceived[] = "<DTMFEvent>";
static const char g_szCallDtmfReceivedEnd[] = "</DTMFEvent>";
static const char g_szCallTransferSuccess[] = "<CallTransferSuccess />";
static const char g_szCallTransferFail[] = "<CallTransferFail>";
static const char g_szCallTransferFailEnd[] = "</CallTransferFail>";
static const char g_szVideophoneError[] = "<VideophoneError />";
static const char g_szNoActiveCall[] = "<NoActiveCall />";
static const char g_szOutgoingCall[] = "<OutgoingCall />";
static const char g_szIncomingCall[] = "<IncomingCall />";
static const char g_szUnknown[] = "<Unknown />";
static const char g_szCallInProgress[] = "<CallInProgress />";
static const char g_szIsAliveAck[] = "<IsAliveAck />";
static const char g_szIsAliveAckPartial[] = "<IsAliveAck"; // Note: The end bracket is intentionally missing!
static const char g_szLocalNameSetSuccess[] = "<LocalNameSetSuccess />";

static const char g_szStatusOnHoldLocal[] = "<StatusOnHold><HoldLocal /></StatusOnHold>";
static const char g_szStatusOnHoldRemote[] = "<StatusOnHold><HoldRemote /></StatusOnHold>";
static const char g_szStatusOnHoldBoth[] = "<StatusOnHold><HoldBoth /></StatusOnHold>";
static const char g_szStatusMultipleCalls[] = "<StatusMultipleCalls />";
static const char g_szStatusIdle[] = "<StatusIdle />";

static const char g_szStatusInCallEx[] = "<StatusInCallEx>";
static const char g_szStatusInCallExEnd[] = "</StatusInCallEx>";
static const char g_szStatusConnecting[] = "<StatusConnecting>";
static const char g_szStatusConnectingEnd[] = "</StatusConnecting>";
static const char g_szStatusLeavingMessage[] = "<StatusLeavingMessage/>";
static const char g_szStatusDisconnecting[] = "<StatusDisconnecting />";
static const char g_szStatusShutdown[] = "<StatusShutdown />";
static const char g_szStatusError[] = "<StatusError />";

static const char g_szError[] = "<Error>";
static const char g_szErrorEnd[] = "</Error>";
static const char g_szWarning[] = "<Warning>";
static const char g_szWarningEnd[] = "</Warning>";

static const char g_szCallResultSuccessful[] = "<CallResultSuccessful />";
static const char g_szCallResultRejected[] = "<CallResultRejected />";
static const char g_szCallResultNotFound[] = "<CallResultNotFound />";
static const char g_szCallResultFindFailed[] = "<CallResultFindFailed />";
static const char g_szCallResultBusy[] = "<CallResultBusy />";
static const char g_szCallResultUnreachable[] = "<CallResultUnreachable />";
static const char g_szCallResultUnknown[] = "<CallResultUnknown />";
static const char g_szCallResultMailBoxFull[] = "<CallResultMailboxFull />";
static const char g_szCallResultDialError[] = "<CallResultDialError />";

static const char g_szExternalConferenceStatus[] = "<ExternalConferenceStatus>";
static const char g_szExternalConferenceStatusEnd[] = "</ExternalConferenceStatus>";
static const char g_szExternalConferenceParticipantCount[] = "<ExternalConferenceParticipantCount>";
static const char g_szExternalConferenceParticipantCountEnd[] = "</ExternalConferenceParticipantCount>";
static const char g_szApplicationCapabilities[] = "Capabilities";
static const char g_szConferences[] = "Conferences";

static const char g_szOnSwitch[] = "On";

#define stiMAX_VRCL_READ_SIZE 512

static const int nAMP_LEN = 5;
static const int nLT_LEN = 4;
static const int nGT_LEN = 4;

const char g_szUNKNOWN[] = "Unknown";

static const char g_szPROTOCOL_SIP[] = "sip";
static const char g_szPROTOCOL_SIPS[] = "sips";
static const char g_szPROTOCOL_UNKOWN[] = "unknown";


/*!
 * \brief Constructor
 */
CstiVRCLServer::CstiVRCLServer (
	CstiConferenceManager *conferenceManager)
:
	CstiEventQueue (stiVRCL_TASK_NAME),
	m_pConferenceManager (conferenceManager),
	m_connectionTimer (nMAX_CONNECTION_TIME, this),
	m_callInProgressTimer (nCALL_IN_PROGRESS_INTERVAL, this),
	m_lifeCheckTimer (nLIFE_CHECK_INTERVAL, this),
	m_messagePlayTimer (0, this)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::CstiVRCLServer);

	m_poPM = PropertyManager::getInstance ();
	m_szLastServerKey[0] = '\0';
	m_szHearingPhoneNumber[0] = '\0';

	m_signalConnections.push_back (m_connectionTimer.timeoutSignal.Connect (
			[this]{ ClientDisconnect(); }));

	m_signalConnections.push_back (m_callInProgressTimer.timeoutSignal.Connect (
			[this]{ eventCallInProgressTimer(); }));

	m_signalConnections.push_back (m_lifeCheckTimer.timeoutSignal.Connect (
		[this]{
			if (m_nClientSocket != stiINVALID_SOCKET)
			{
				EstiResult eResult = ClientResponseSend (g_szIsAliveMessage);
				if (estiOK != eResult) { ClientDisconnect (); }
				else
				{
					// Have we exceeded the allowable number of pending responses?
					if (++m_unPendingLifeCheckResponses > 1)
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::eventLifeCheckTimer: No life response from client!\n"); );
						ClientDisconnect ();
					}
				}
			}
			m_lifeCheckTimer.restart ();
		}));

}


/*!
 * \brief Destructor
 */
CstiVRCLServer::~CstiVRCLServer ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::~CstiVRCLServer);

	if (m_audioOutputDtmfReceivedSignalConnection)
	{
		m_audioOutputDtmfReceivedSignalConnection.Disconnect ();
	}

	Shutdown ();
}


/*!
 * \brief Initializes the VRCL Server
 */
stiHResult CstiVRCLServer::Initialize (
	const std::string &version)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::Initialize);
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	m_applicationVersion = version;

	IstiAudioOutput *istiAudioOutput = IstiAudioOutput::InstanceGet();
	stiASSERT (istiAudioOutput);
	if (istiAudioOutput)
	{
		m_audioOutputDtmfReceivedSignalConnection =
			istiAudioOutput->dtmfReceivedSignalGet ().Connect (
				[this](EstiDTMFDigit digit){
		            PostEvent ([this,digit]{ eventCallDtmfReceived(digit); });
				});
	}

	return hResult;
}


stiHResult CstiVRCLServer::Startup ()
{
	//
	// Initialize the circular message buffer for handling client messages
	//
	m_oMessageBuffer.Open (nullptr, stiMAX_VRCL_READ_SIZE * 2);

	//
	// Create TCP/IP based listening socket
	//
	ListenSocketOpen ();

	StartEventLoop ();

	return stiRESULT_SUCCESS;
}



/*!
 * \brief Handler that reads data from client socket when
 *        the event loop determines it can be read from
 */
void CstiVRCLServer::eventReadClientData (
	int clientSocket)
{
	std::vector<char> buf(stiMAX_VRCL_READ_SIZE, 0);

	// Read the data from the socket
	int numBytes = stiOSRead (clientSocket, buf.data(), buf.size());
	if (numBytes > 0)
	{
		EstiResult result = ClientMessageProcess (buf.data(), numBytes);
		if (result != estiOK)
		{
			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				stiTrace ("CstiVRCLServer::eventReadDataFromClient - Error Processing "
					"Client Message, Closing.\n");
			);
			ClientDisconnect ();
		}
	}
	else
	{
		// No! Close the socket connection.
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("CstiVRCLServer::eventReadDataFromClient - Error reading from "
			"socket, Closing.\n");
		);
		ClientDisconnect ();
	}
}


/*!
 * \brief Event handler for accepting new connections on
 *        a given listening socket.
 */
void CstiVRCLServer::eventAcceptNewConnection (
	int listenSocket)
{
	stiDEBUG_TOOL (g_stiVRCLServerDebug,
		stiTrace ("CstiVRCLServer::eventAcceptNewConnection - Received Client Connect.\n");
	);

	sockaddr_in sockAddrIn = {};
	int sockAddrInSize = sizeof sockAddrIn;

	int newSocket = ::accept (
		listenSocket,
		(sockaddr *)&sockAddrIn,
		(socklen_t*)&sockAddrInSize);

	// Do nothing if the new socket is invalid
	if (newSocket == stiINVALID_SOCKET)
	{
		return;
	}

	stiDEBUG_TOOL (g_stiVRCLServerDebug,
		stiTrace ("CstiVRCLServer::eventAcceptNewConnection - Client connected "
		"successfully.\n");
	);

	// Can we add the client to the client array?
	if (estiOK == ClientAdd(newSocket))
	{
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("CstiVRCLServer::eventAcceptNewConnection - Client added "
			"to connected client list successfully.\n");
		);
		// Call CCI with the VRS Call Id as an empty string.
		vrsCallIdSetSignal.Emit (std::string());
	}
	else
	{
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("CstiVRCLServer::eventAcceptNewConnection - "
			          "Client could not be added to connected client list.\n");
		);
		stiOSClose (newSocket);
	}
}	


/*!
 * \brief Close down all sockets in use
 */
stiHResult CstiVRCLServer::ListenSocketClose ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	// Disconnect any clients
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		stiOSClose (m_nClientSocket);
		m_nClientSocket = stiINVALID_SOCKET;
		m_bAuthenticated = estiFALSE;

		// Inform the application layer of the closure
		connectionClosedSignal.Emit ();
	}
	
	// Stop IPv4
	if (m_nIPv4ListeningSocket != stiINVALID_SOCKET)
	{
		// Stop monitoring this file descriptor in the event loop
		FileDescriptorDetach (m_nIPv4ListeningSocket);
		stiOSClose (m_nIPv4ListeningSocket);
		m_nIPv4ListeningSocket = stiINVALID_SOCKET;
	}
	
	// Stop IPv6
	if (m_nIPv6ListeningSocket != stiINVALID_SOCKET)
	{
		// Stop monitoring this file descriptor in the event loop
		FileDescriptorDetach (m_nIPv6ListeningSocket);
		stiOSClose (m_nIPv6ListeningSocket);
		m_nIPv6ListeningSocket = stiINVALID_SOCKET;
	}
	
	return hResult;
}


/*!
 * \brief Opens a single VRCL Server listen port
 */
stiHResult CstiVRCLServer::IpSocketOpen (
	int Family, ///! Tells the function which family of socket to open (AF_INET or AF_INET6)
	int *socket)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Select an appropriate settings structure
	sockaddr_in socketAddressV4{};
	sockaddr_in6 socketAddressV6{};
	nonstd::observer_ptr<sockaddr> socketAddress;
	int sockaddrSize{0};
	
	if (Family == AF_INET)
	{
		sockaddrSize = sizeof (socketAddressV4);
		socketAddressV4.sin_port = htons (m_un16ListeningPort);
		socketAddressV4.sin_addr.s_addr = htonl (INADDR_ANY);
		socketAddress = nonstd::make_observer(reinterpret_cast<sockaddr *>(&socketAddressV4));
		socketAddress->sa_family = Family;
	}
	else if (Family == AF_INET6)
	{
		sockaddrSize = sizeof (socketAddressV6);
		socketAddressV6.sin6_port = htons (m_un16ListeningPort);
		socketAddressV6.sin6_addr = in6addr_any;
		socketAddress = nonstd::make_observer(reinterpret_cast<sockaddr *>(&socketAddressV6));
		socketAddress->sa_family = Family;
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

	// Yes! If we are not already initialized...
	if (stiINVALID_SOCKET == *socket)
	{
		// Create a socket for listening
		if ((*socket = stiOSSocket (Family, SOCK_STREAM, IPPROTO_TCP))
			!= stiINVALID_SOCKET)
		{
			// Setup socket options.  Tell the socket to close when close is
			// called even if there is still data to send.
			struct linger stLinger{};

			stLinger.l_onoff = 0;
			stLinger.l_linger = 0;

			if (stiERROR == setsockopt (*socket, SOL_SOCKET,
				SO_LINGER, (char *)&stLinger, sizeof (stLinger)))
			{
				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					stiTrace ("CstiVRCLServer::IpSocketOpen - "
					"There was an error "
					"setting the SO_LINGER option on the server socket.\n");
				);
			} // end if

			// Tell the socket to be able to be reused immediately.
			int yes = 1;

			if (stiERROR == setsockopt (*socket, SOL_SOCKET,
				SO_REUSEADDR, (char *)&yes, sizeof (yes)))
			{
				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					stiTrace ("CstiVRCLServer::IpSocketOpen - "
					"There was an error "
					"setting the SO_REUSEADDR option on the server socket.\n");
				);
			} // end if
			
#ifdef IPV6_ENABLED
			// Setting this flag to YES, turns OFF any ipv4->ipv6 mapping mechanisms in play
			// and therefore allows our own ipv4 connection on the same port to work.
#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
			if (Family == AF_INET6)
			{
				setsockopt (*socket, IPPROTO_IPV6, IPV6_V6ONLY, &yes, sizeof (yes));
			}
#elif APPLICATION == APP_NTOUCH_PC
			// TODO: Implement for ntouchPC (WIN32)
#else
			if (Family == AF_INET6)
			{
				setsockopt (*socket, SOL_IPV6, IPV6_V6ONLY, &yes, sizeof (yes));
			}
#endif // APPLICATION == APP_NTOUCH_MAC || APP_NTOUCH_IOS
#endif // IPV6_ENABLED

			if (bind (*socket,
				socketAddress.get (),
				sockaddrSize) != stiERROR)
			{
				if (listen (*socket, 1) != stiERROR)
				{
					hResult = stiRESULT_SUCCESS;

					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::IpSocketOpen - "
						"Server Created successfully.\n Listening on IPv%d"
						"socket: %ld        Port: %d\n",
						Family == AF_INET6 ? 6 : 4,
								*socket, m_un16ListeningPort);
					);
				} // end if
				else
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::IpSocketOpen - "
							"Server could not be started.  "
							"Call to listen failed - errno(%d)\n", errno);
					);
					stiTHROW (stiRESULT_ERROR);
				} // end else
			} // end if
			else
			{
				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					stiTrace ("CstiVRCLServer::IpSocketOpen - "
						"Server could not be started.  "
						"Call to bind failed - errno(%d)\n", errno);
				);
				stiTHROW (stiRESULT_ERROR);
			} // end else
		} // end if
		else
		{
			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				stiTrace ("CstiVRCLServer::IpSocketOpen - "
					"Server could not be started.  Socket init failed\n");
			);
			stiTHROW (stiRESULT_ERROR);
		} // end else
	} // end if

STI_BAIL:

	return hResult;
}


/*!
 * \brief Opens all relevant VRCL server listen ports.  The caller must
 *        have the exec mutex locked prior to calling this method.
 */
stiHResult CstiVRCLServer::ListenSocketOpen ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::ListenSocketOpen);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Do we have a valid port?
	if (m_un16ListeningPort != 0)
	{
		hResult = IpSocketOpen (AF_INET, &m_nIPv4ListeningSocket);
		
#ifdef IPV6_ENABLED
		stiHResult hResult2 = IpSocketOpen (AF_INET6, &m_nIPv6ListeningSocket);
		if (stiIS_ERROR (hResult) && stiIS_ERROR (hResult2))
		{
			stiTESTRESULT ();
		}
#else
		stiTESTRESULT ();
#endif // IPV6_ENABLED

		// Monitor the new listening socket(s) in the event loop
		// so we can accept new connections
		if (m_nIPv4ListeningSocket != stiINVALID_SOCKET)
		{
			if (!FileDescriptorAttach (
				m_nIPv4ListeningSocket,
				[this]{ eventAcceptNewConnection(m_nIPv4ListeningSocket); }))
			{
				stiASSERTMSG(estiFALSE, "%s: can't attach IPv4ListeningSocket %d\n", __func__, m_nIPv4ListeningSocket);
			}
		}

		if (m_nIPv6ListeningSocket != stiINVALID_SOCKET)
		{
			if (!FileDescriptorAttach (
				m_nIPv6ListeningSocket,
				[this]{ eventAcceptNewConnection(m_nIPv6ListeningSocket); }))
			{
				stiASSERTMSG(estiFALSE, "%s: can't attach IPv6ListeningSocket %d\n", __func__, m_nIPv6ListeningSocket);
			}
		}
	}
	else
	{
		// No! Our port is zero which means we don't open the socket.
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("CstiVRCLServer::ListenSocketOpen - "
			"Server won't be started since socket is 0.\n");
		);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		// Since we have failed to open a socket, we are going to close it
		// and change the member variable back to error so that we don't
		// select on it in the main task loop.
		ListenSocketClose ();
	}

	return (hResult);
}


/*!
 * \brief Sets the listenting port for the VRCL Server
 */
stiHResult CstiVRCLServer::PortSet (
    uint16_t un16Port)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::PortSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	std::lock_guard<std::recursive_mutex> lock (m_execMutex);

	// Has the port changed?
	if (m_un16ListeningPort != un16Port)
	{
		m_un16ListeningPort = un16Port;
		PostEvent ([this]{ eventChangePort(); });
	}

	return hResult;
}


/*!
 * \brief Retrieves the listening port for the VRCL Server, or 0 on error
 */
uint16_t CstiVRCLServer::PortGet ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::PortGet);

	int port = 0;
	m_poPM->propertyGet (cmVRCL_PORT, &port);

	return (uint16_t)port;
}


void CstiVRCLServer::callDialError ()
{
	std::stringstream message;

	message << g_szCallTerminated << g_szCallResultDialError << g_szCallTerminatedEnd;

	auto result = ClientResponseSend (message.str ().c_str ());
	if (estiOK != result)
	{
		ClientDisconnect ();
	}
}


/*!
 * \brief Notifies the VRCL system about application notifications
 */
EstiResult CstiVRCLServer::VRCLNotify (
    EstiCmMessage msg)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::VRCLNotify(msg));

	EstiResult eResult = estiOK;

	switch (msg)
	{
		
//	    case estiMSG_UPDATE_UP_TO_DATE:
//		case estiMSG_UPDATE_ERROR:
//		case estiMSG_UPDATE_SUCCESSFUL:
//		    UpdateCheckResult (msg);
//			break;

		case estiMSG_HEARING_STATUS_SENT:
			HearingStatusResponse();
			break;

		case estiMSG_NEW_CALL_READY_SENT:
			RelayNewCallReadyResponse();
			break;

	    case estiMSG_SIGNMAIL_AVAILABLE:
			PostEvent ([this]{ eventSignMailAvailable(); });
		    break;

	    case estiMSG_SIGNMAIL_RECORDING_STARTED:
		    // If the API version is 3.1 or later ...
			PostEvent ([this]{ eventSignMailRecordingStarted(); });
		    break;

	    case estiMSG_REMOVE_TEXT:
	        {
		        PostEvent ([this]{ eventTextMessageCleared(); });
	        }
		    break;

	    case estiMSG_HEARING_VIDEO_CONNECTED:
	        {
		        PostEvent ([this]{ eventHearingVideoConnected(); });
	        }
		    break;

	    case estiMSG_HEARING_VIDEO_CONNECTION_FAILED:
	        {
		        PostEvent ([this]{ eventHearingVideoConnectionFailed(); });
	        }
		    break;

	    default:
		    // do nothing
		    eResult = estiOK;
		    break;
	    }

	return (eResult);
} // end CstiVRCLServer::VRCLNotify


/*!
 * \brief Notifies the VRCL system about application notifications
 */
EstiResult CstiVRCLServer::VRCLNotify (
    EstiCmMessage msg,
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::VRCLNotify);

	EstiResult eResult = estiOK;

	switch (msg)
	{
		case estiMSG_INCOMING_CALL: // CallIncoming
			eResult = CallIncomingEventSend (call);
			break;

		case estiMSG_RESOLVING_NAME: // Performing a directory resolve
			eResult = StateSend (call);		// Terpnet needs to know quickly that we are working on their call.
														// It can't wait for the resolve to return since that can sometimes take too long.
			break;

		case estiMSG_CONFERENCING: // CallConnected
			eResult = CallConnectedEventSend (call);
			break;

		case estiMSG_DIALING:
			eResult = CallDialedEventSend (call);
			break;

		case estiMSG_LEAVE_MESSAGE:
			// Greeting is starting for leaving a message
			// Tell the client that we are beginning the greeting
			if (call->GreetingStartingGet())
			{
				PostEvent ([this]{ eventSignMailGreetingStarted(); });
			}
			break;

		case estiMSG_DISCONNECTED: // Idle state

			if (call->SubstateGet() == estiSUBSTATE_NONE)
			{
				eResult = CallTerminatedEventSend (call);

				// Clear any previous hearing phone number, since it is no
				// longer useful.
				m_szHearingPhoneNumber[0] = '\0';
			}

			break;

		default:
			// do nothing
			eResult = estiOK;
			break;
		}

	return (eResult);
} // end CstiVRCLServer::VRCLNotify


/*!
 * \brief Notifies the VRCL system about application notifications
 */
EstiResult CstiVRCLServer::VRCLNotify (
    EstiCmMessage msg,
    size_t param)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::VRCLNotify);

	EstiResult eResult = estiOK;

	switch (msg)
	{
		case estiMSG_CB_ERROR_REPORT:
			eResult = ErrorSend ((SstiErrorLog*)param);
			break;

	    case estiMSG_HEARING_VIDEO_CAPABLE:
	        {
				auto hearingVideoCapable = static_cast<bool>(param);
		        PostEvent ([this, hearingVideoCapable]{ eventHearingVideoCapable(hearingVideoCapable);});
	        }
		    break;

		case estiMSG_HEARING_VIDEO_DISCONNECTED:
	        {
				auto hearingDisconnected = static_cast<bool>(param);
		        PostEvent ([this, hearingDisconnected]{ eventHearingVideoDisconnected(hearingDisconnected); });
	        }
		    break;

	    default:
		    // do nothing
		    eResult = estiOK;
		    break;
	    }

	return (eResult);
} // end CstiVRCLServer::VRCLNotify

/*!
 * \brief Notifies the VRCL system about application notifications
 */
EstiResult CstiVRCLServer::HearingVideoConnecting (
	const std::string &conferenceURI,
	const std::string &initiator,
	const std::string &profileId)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::VRCLNotify);

	EstiResult eResult = estiOK;

	PostEvent ([this, conferenceURI, initiator, profileId]
		{ 
			eventHearingVideoConnecting(conferenceURI, initiator, profileId); 
		});

	return (eResult);
} // end CstiVRCLServer::VRCLNotify


/*!
 * \brief Sends the "Call Dialed" event to the client
 */
EstiResult CstiVRCLServer::CallDialedEventSend (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::CallDialedEventSend);
	if (!call)
	{
		return estiERROR;
	}

	PostEvent ([this, call]{ eventCallDialed (call); });
	return estiOK;
}


/*!
 * \brief Sends the "Incoming Call" event to the client
 */

EstiResult CstiVRCLServer::CallIncomingEventSend (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::CallIncomingEventSend);
	if (!call)
	{
		return estiERROR;
	}

	PostEvent ([this,call]{ eventCallIncoming(call); });
	return estiOK;
}


/*!
 * \brief Sends the "Call Connected" event to the client
 */
EstiResult CstiVRCLServer::CallConnectedEventSend (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::CallConnectedEventSend);
	if (!call)
	{
		return estiERROR;
	}

	PostEvent ([this,call]{ eventCallConnected(call); });
	return estiOK;
}


/*!
 * \brief Sends the "Call Terminated" event to the client
 */
EstiResult CstiVRCLServer::CallTerminatedEventSend (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::CallTerminatedEventSend);
	if (!call)
	{
		return estiERROR;
	}

	PostEvent ([this,call]{ eventCallTerminated(call); });
	return estiOK;
}


/*!
 * \brief Notify about bridge state changes
 */
stiHResult CstiVRCLServer::BridgeStateChange (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::BridgeStateChange);

	// Are we currently connected?
	if (call && IsConnectedCheck())
	{
		PostEvent ([this,call]{ eventBridgeStateChange(call); });
	}

	return stiRESULT_SUCCESS;
}


/*!
 * \brief Notifies about call bridge state changes
 */
void CstiVRCLServer::eventBridgeStateChange (
    VRCLCallSP call)
{
	// Do nothing if the call pointer is invalid
	if (!call)
	{
		return;
	}

	if (IsConnectedCheck())
	{
		EsmiCallState eBridgeState = esmiCS_UNKNOWN;
		call->BridgeStateGet (&eBridgeState);
		switch (eBridgeState)
		{
		    case esmiCS_IDLE:
		    case esmiCS_CONNECTING:
			    // No care
			    break;

		    case esmiCS_CONNECTED:
			    // Success
			    ClientResponseSend (g_szCallBridgeSuccess);
				call->BridgeCompletionNotifiedSet(true);
			    break;

		    case esmiCS_HOLD_LCL:
		    case esmiCS_HOLD_RMT:
		    case esmiCS_HOLD_BOTH:
		    case esmiCS_CRITICAL_ERROR:
		    case esmiCS_INIT_TRANSFER:
		    case esmiCS_TRANSFERRING:
		    {
			    char szMessage[sizeof (g_szCallBridgeFail) + sizeof (g_szCallBridgeFailEnd) + 1];
				// error
				sprintf (
				    szMessage,
				    "%s%s",
				    g_szCallBridgeFail,
				    g_szCallBridgeFailEnd);
				ClientResponseSend (szMessage);
				break;
		    }

			//If bridge state is unknown it is because BridgeStateGet() failed so the bridge is no longer connected.
		    case esmiCS_UNKNOWN:
			    break;

		    case esmiCS_DISCONNECTING:
		    case esmiCS_DISCONNECTED:
			    if (!call->BridgeCompletionNotifiedGet ())
				{
					char szMessage[sizeof (g_szCallBridgeFail) + sizeof (g_szCallBridgeFailEnd) + 1];
					// error
					sprintf (
					    szMessage,
					    "%s%s",
					    g_szCallBridgeFail,
					    g_szCallBridgeFailEnd);
					ClientResponseSend (szMessage);
				}
				else
				{
					ClientResponseSend (g_szCallBridgeDisconnect);
				}
			    break;

			// default: // commented out for type safety
		}
	}
}


/*!
 * \brief Notify client about state changes
 */
EstiBool CstiVRCLServer::StateChangeNotify (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::StateChangeNotify);

	// Are we currently connected?
	if (call && IsConnectedCheck())
	{
		PostEvent ([this,call]{ eventStateChange(call); });
	}

	return estiTRUE;
}


/*!
 * \brief Send an error message to the client
 */
EstiResult CstiVRCLServer::ErrorSend (
    const SstiErrorLog *errorLog)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::ErrorSend);

	EstiResult result = estiOK;

	// Are we currently connected?
	if (IsConnectedCheck ())
	{
		std::shared_ptr<SstiErrorLog> errorLogCopy = std::make_shared<SstiErrorLog>(*errorLog);

		PostEvent ([this,errorLogCopy]{ eventError(errorLogCopy); });

		// This is confusing...  error on success?
		result = estiERROR;
	}
	return result;
}


/*!
 * \brief Event handler for when a call has been dialed
 */
void CstiVRCLServer::eventCallDialed (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventCallDialed);
	
	// Do nothing if the call object is invalid
	if (!call)
	{
		return;
	}
	
	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a response to the Client that we have dialed a call
		char message[nMAX_MESSAGE_BUFFER];

		std::string dialString;
		call->RemoteDialStringGet (&dialString);

		sprintf (
		    message,
		    "%s%s%s",
		    g_szCallDialed,
		    dialString.c_str (),
		    g_szCallDialedEnd);

		EstiResult eResult = ClientResponseSend (message);
		if (estiOK != eResult)
		{
			ClientDisconnect ();
		}
	}
}


/*!
 * \brief Event handler for an incoming call
 */
void CstiVRCLServer::eventCallIncoming (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventCallIncoming);
	
	// Do nothing if the call object is invalid
	if (!call)
	{
		return;
	}
	
	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client that we have an incoming call
		char message[nMAX_MESSAGE_BUFFER];
		char tmpBuf[nMAX_MESSAGE_BUFFER];
		int tmpBufSize = nMAX_MESSAGE_BUFFER;

		std::string caller;

		call->RemoteNameGet (&caller);

		sprintf (
		    message,
		    "%s%s%s",
		    g_szCallIncoming,
		    strXescape (caller.c_str (), tmpBuf, &tmpBufSize),
		    g_szCallIncomingEnd);

		EstiResult eResult = ClientResponseSend (message);
		if (estiOK != eResult)
		{
			ClientDisconnect ();
		}
	}
}


/*!
 * \brief Event handler for when a call gets connected
 */
void CstiVRCLServer::eventCallConnected (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventCallConnected);

	// Do nothing if the call object is invalid
	if (!call)
	{
		return;
	}

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		m_bCallInProgress = estiTRUE;

		// Send a message to the Client that we have an incoming call
		std::stringstream message;

		// Add the beginning tag...
		message << g_szCallConnectedEx;

		// Add the user's data
		UserInfoAdd (call, &message);

		// Add the ending tag
		message << g_szCallConnectedExEnd;

		// Send the message to the client.
		ClientResponseSend (message.str().c_str());

		// Notifies the client that we are in a call
		m_callInProgressTimer.restart ();
	}
}


/*!
 * \brief Event handler for when a call is terminated
 */
void CstiVRCLServer::eventCallTerminated (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventCallTerminated);
	
	// Stop the call in progress timer, since we are no longer connected.
	m_bCallInProgress = estiFALSE;
	m_callInProgressTimer.stop ();

	if (!call)
	{
		return;
	}

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client that we have terminated the call.
		char szMessage[nMAX_MESSAGE_BUFFER];
		const char* pszCallResult = "";

	   if (call->SignMailInitiatedGet() && 
		   call->signMailBoxFullGet())
	    {
		   pszCallResult = g_szCallResultMailBoxFull;
	    }
		else
		{
			// Build the message to send based on the call result.
			switch (call->ResultGet ())
			{
				case estiRESULT_CALL_SUCCESSFUL:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_CALL_SUCCESSFUL\n");
					);
					pszCallResult = g_szCallResultSuccessful;
					break;

				case estiRESULT_NOT_FOUND_IN_DIRECTORY:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_NOT_FOUND_IN_DIRECTORY\n");
					);
					pszCallResult = g_szCallResultNotFound;
					break;

				case estiRESULT_DIRECTORY_FIND_FAILED:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_DIRECTORY_FIND_FAILED\n");
					);
					pszCallResult = g_szCallResultFindFailed;
					break;

				case estiRESULT_REMOTE_SYSTEM_REJECTED:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_REMOTE_SYSTEM_REJECTED\n");
					);
					pszCallResult = g_szCallResultRejected;
					break;

				case estiRESULT_REMOTE_SYSTEM_BUSY:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_REMOTE_SYSTEM_BUSY\n");
					);
					pszCallResult = g_szCallResultBusy;
					break;

				case estiRESULT_REMOTE_SYSTEM_UNREACHABLE:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_REMOTE_SYSTEM_UNREACHABLE\n");
					);
					pszCallResult = g_szCallResultUnreachable;
					break;

				case estiRESULT_LOCAL_SYSTEM_REJECTED:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_LOCAL_SYSTEM_REJECTED\n");
					);
					pszCallResult = g_szCallResultRejected;
					break;

				case estiRESULT_LOCAL_SYSTEM_BUSY:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_LOCAL_SYSTEM_BUSY\n");
					);
					pszCallResult = g_szCallResultRejected;
					break;
				case estiRESULT_UNKNOWN:
				default:
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::eventCallTerminated - Result: estiRESULT_UNKNOWN\n");
					);
					pszCallResult = g_szCallResultUnknown;
				break;
			} // end switch
		}

		sprintf (
		    szMessage,
		    "%s%s%s",
		    g_szCallTerminated,
		    pszCallResult,
		    g_szCallTerminatedEnd);

		auto result = ClientResponseSend (szMessage);
		if (estiOK != result)
		{
			ClientDisconnect ();
		}
	}
}


/*!
 * \brief Event handler for when a signmail is available
 */
void CstiVRCLServer::eventSignMailAvailable ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventSignMailAvailable);
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);

	// TerpNet Signmail isn't available until 3.1 or later.  This should have already been tested
	// in the method that submitted this event.
	
	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client.
		char szMessage[nMAX_MESSAGE_BUFFER];
		sprintf (szMessage, "<%s />", g_szSignMailAvailable);
		
		stiTESTCOND (estiOK == ClientResponseSend(szMessage), stiRESULT_ERROR);
	}

STI_BAIL:
	return;
}


/*!
 * \brief Event handler for when a signmail greeting is started
 */
void CstiVRCLServer::eventSignMailGreetingStarted ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventSignMailGreetingStarted);

	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);

	// TerpNet Signmail isn't available until 3.1 or later.  This should have already been tested
	// in the method that submitted this event.
	
	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client.
		char szMessage[nMAX_MESSAGE_BUFFER];
		sprintf (szMessage, "<%s />", g_szSignMailGreetingStarted);
		
		stiTESTCOND (estiOK == ClientResponseSend(szMessage), stiRESULT_ERROR);
	}

STI_BAIL:
	return;
}


void CstiVRCLServer::eventTextMessageCleared ()
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client.
		char szMessage[nMAX_MESSAGE_BUFFER];
		sprintf (szMessage, "<%s />", g_szTextMessageCleared);

		stiTESTCOND (estiOK == ClientResponseSend(szMessage), stiRESULT_ERROR);
	}

STI_BAIL:
	return;
}


void CstiVRCLServer::eventSignMailRecordingStarted ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventSignMailRecordingStarted);
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);

	// TerpNet Signmail isn't available until 3.1 or later.  This should have already been tested
	// in the method that submitted this event.
	
	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client.
		char szMessage[nMAX_MESSAGE_BUFFER];
		sprintf (szMessage, "<%s />", g_szSignMailRecordingStarted);
		
		stiTESTCOND (estiOK == ClientResponseSend(szMessage), stiRESULT_ERROR);
	}
	
STI_BAIL:
	return;
}


void CstiVRCLServer::eventCallDtmfReceived (
    EstiDTMFDigit digit)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventCallDtmfReceived);
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Convert from the enum to a char
		char c = '\0';
		switch (digit)
		{
		    case estiDTMF_DIGIT_0:  c = '0';  break;
		    case estiDTMF_DIGIT_1:  c = '1';  break;
		    case estiDTMF_DIGIT_2:  c = '2';  break;
		    case estiDTMF_DIGIT_3:  c = '3';  break;
		    case estiDTMF_DIGIT_4:  c = '4';  break;
		    case estiDTMF_DIGIT_5:  c = '5';  break;
		    case estiDTMF_DIGIT_6:  c = '6';  break;
		    case estiDTMF_DIGIT_7:  c = '7';  break;
		    case estiDTMF_DIGIT_8:  c = '8';  break;
		    case estiDTMF_DIGIT_9:  c = '9';  break;
		    case estiDTMF_DIGIT_S:  c = '*';  break;
		    case estiDTMF_DIGIT_P:  c = '#';  break;
		    default:
			    stiTESTCOND (estiFALSE, stiRESULT_ERROR);
			    break;
		}

		// Send a message to the Client that we have termintated the call.
		char message[nMAX_MESSAGE_BUFFER];
		sprintf (message, "%s%c%s", g_szCallDtmfReceived, c, g_szCallDtmfReceivedEnd);

		stiTESTCOND (estiOK == ClientResponseSend(message), stiRESULT_ERROR);
	}

STI_BAIL:
	return;
}


/*!
 * \brief Event handler to change the listen port for incoming connections
 */
void CstiVRCLServer::eventChangePort ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventChangePort);

	ListenSocketClose ();
	ListenSocketOpen();
}


/*!
 * \brief Returns true if the client version is >= to passed in version
 */
EstiBool CstiVRCLServer::ClientVersionCheck (
	int nMajor,
	int nMinor)
{
	EstiBool bResult = estiFALSE;
	
	if (m_nClientAPIMajorVersion > nMajor ||
		(m_nClientAPIMajorVersion == nMajor &&
		m_nClientAPIMinorVersion >= nMinor))
	{
		bResult = estiTRUE;
	}
	
	return bResult;
}


/*!
 * \brief Event handler for when the callInProgress timer fires
 */
void CstiVRCLServer::eventCallInProgressTimer ()
{
	// If we are connected to a client and there is a call in progress...
	if (stiINVALID_SOCKET != m_nClientSocket &&
	    m_bCallInProgress)
	{
		bool sent = false;

		// If the engine is in the "Ready" state and we have only a single call object,
		// check to see if it is in a "Hold" state.  If it is, send the "hold" state
		// instead of sending the "In Progress" message.
		if (m_pConferenceManager->eventLoopIsRunning() &&
		    1 == m_pConferenceManager->CallObjectsCountGet())
		{
			auto call = callObjectHeadGet();			
			if (call)
			{
				// We have the call object, what state is it in?
				switch (call->StateGet ())
				{
				    case esmiCS_HOLD_LCL:
				    case esmiCS_HOLD_RMT:
				    case esmiCS_HOLD_BOTH:
					    if (estiOK == StateSend(call))
						{
							sent = true;
						}
					    break;

				    default:
					    break;
				}
			}
		}

		// If we haven't already done so, send a response.  In this case, the response
		// is simply an "In Progress" response.
		if (!sent)
		{
			EstiResult eResult = ClientResponseSend (g_szCallInProgress);
			if (estiOK != eResult)
			{
				ClientDisconnect ();
			}
		}

		m_callInProgressTimer.restart ();
	}
}


/*!
 * \brief Event handler for reporting an error
 */
void CstiVRCLServer::eventError (
    std::shared_ptr<SstiErrorLog> errorLog)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventError);

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		const char *type = g_szError;
		const char *typeEnd = g_szErrorEnd;

		if (estiLOG_WARNING == errorLog->eCriticality)
		{
			type = g_szWarning;
			typeEnd = g_szWarningEnd;
		}

		// Format a message including the error number
		char message[nMAX_MESSAGE_BUFFER];
		sprintf (
		    message,
		    "%s%u%s",
		    type,
		    errorLog->un32StiErrorNbr,
		    typeEnd);

		// Send a message to the Client that we have had an error
		EstiResult result = ClientResponseSend (message);
		if (estiOK != result)
		{
			ClientDisconnect ();
		}
	}
}


/*!
 * \brief Will shutdown the VRCL server
 */
stiHResult CstiVRCLServer::Shutdown ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::Shutdown);
	stiHResult hResult = stiRESULT_SUCCESS;

	// Close down the sockets
	ListenSocketClose ();

	// Purge the circular buffer to clear out any old messages.
	m_oMessageBuffer.Purge ();

	// Close the circular message buffer for handling client messages
	m_oMessageBuffer.Close ();

	// Call the base class shutdown.
	StopEventLoop ();

	return hResult;
}


/*!
 * \brief Event handler for state changes
 */
void CstiVRCLServer::eventStateChange (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventStateChange);
	if (!call)
	{
		return;
	}

	// If we are connected to a client
	if (stiINVALID_SOCKET != m_nClientSocket &&
	    estiOK != StateSend(call))     // message sent successfully?
	{
		ClientDisconnect ();
	}
}


/*!
 * \brief Sends the VRCL API version
 */
EstiResult CstiVRCLServer::ApiVersionSend ()
{
	stiLOG_ENTRY_NAME(CstiVRCLServer::ApiVersionSend);
	
	EstiResult eResult = estiOK;
	stiHResult hResult = stiRESULT_SUCCESS;

	// Can we write the response to the client?
	auto pszResponse = new char[sizeof (g_szAPIVersionMessage) + strlen (g_szAPIVersion) + sizeof (g_szAPIVersionMessageEnd) + 1];
	stiTESTCOND(pszResponse, stiRESULT_ERROR);

	sprintf (pszResponse, "%s%s%s",
		g_szAPIVersionMessage,
		g_szAPIVersion,
		g_szAPIVersionMessageEnd);

	eResult = ClientResponseSend (pszResponse);
	
	if (estiOK != eResult)
	{
		// No! Disconnect them now.
		ClientDisconnect ();
		stiTHROW(stiRESULT_ERROR);
	}


STI_BAIL:	

	if (pszResponse) 
	{
		delete[] pszResponse;
		pszResponse = nullptr;
	}
	 
	return (stiIS_ERROR(hResult) ? estiERROR : estiOK);
}


/*!
 * \brief Sends the application version
 */
EstiResult CstiVRCLServer::ApplicationVersionSend ()
{
	stiLOG_ENTRY_NAME(CstiVRCLServer::ApplicationVersionSend);

	EstiResult eResult = estiOK;

	// Is the application version number set?
	if (m_applicationVersion.length() > 0)
	{
		// Yes! Build the response
		char szResponse[256];

		// A versioning consists of a major and minor part
		// as well as the build number and minor build number.

		// Put the versioning information into a string that can be
		// returned.
		sprintf (szResponse, "%s%s%s",
			g_szApplicationVersion,
		    m_applicationVersion.c_str(),
			g_szApplicationVersionEnd);

		// Can we write the response to the client?
		EstiResult eResult = ClientResponseSend (szResponse);

		if (estiOK != eResult)
		{
			// No! Disconnect them now.
			ClientDisconnect ();
		}
	}
	else
	{
		// No! Return with an error.
		eResult = estiERROR;
	}

	return eResult;
}

/*!
 * \brief Sends the application version
 */
stiHResult CstiVRCLServer::ApplicationCapabilitiesSend()
{
	stiLOG_ENTRY_NAME(CstiVRCLServer::ApplicationCapabilitiesSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Are we connected to a client
	//
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		std::ostringstream message;
		message << "<" << g_szApplicationCapabilities << ">";
		message << "\t<" << g_szConferences << ">";
		message << "\t</" << g_szConferences << ">";
		message << "</" << g_szApplicationCapabilities << ">";

		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND(estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:


	return hResult;
}


/*!
 * \brief Creates a new client connection
 */
EstiResult CstiVRCLServer::ClientAdd (
	int32_t nSocket)    // Socket for new client
{
	stiLOG_ENTRY_NAME(CstiVRCLServer::ClientAdd);

	EstiResult  eResult = estiOK;

	// Is there already a client connected?
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Yes! We can only accept one client at a time!
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace("CstiVRCLServer::ClientAdd: Attempting to connect a "
			"second client!.\n");
		);
		eResult = estiERROR;
	}
	else
	{
		// No! Connect the client now.

		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace("CstiVRCLServer::ClientAdd: Adding client... \n");
		);

		// Reset the client API version
		m_nClientAPIMajorVersion = 0;
		m_nClientAPIMinorVersion = 0;

		// Reset the pending Life Responses
		m_unPendingLifeCheckResponses = 0;

		int         nOptVal = 0;
		struct      sockaddr_in stInSocketAddress{};  // contains info for remote client
		int         nInSocketAddressLen;           // length of struct

		nInSocketAddressLen = sizeof (stInSocketAddress);

		if (stiINVALID_SOCKET == nSocket)
		{
			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				stiTrace("CstiVRCLServer::ClientAdd Invalid Socket.\n");
			);
			eResult = estiERROR;
		}
		else
		{
			// Get ip from the client socket descriptor
			if (stiERROR != getpeername (nSocket,
				(struct sockaddr *)&stInSocketAddress, (socklen_t *)&nInSocketAddressLen))
			{
				memcpy(&m_un32ClientIPAddress,&stInSocketAddress.sin_addr,
					sizeof(m_un32ClientIPAddress));

				m_un32ClientIPAddress = htonl(m_un32ClientIPAddress);

				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					char addressString[INET6_ADDRSTRLEN];
					inet_ntop (stInSocketAddress.sin_family, &stInSocketAddress.sin_addr, addressString, sizeof (addressString));
					stiTrace("CstiVRCLServer::ClientAdd new client IP "
						"address: %s\n", addressString);
				); // end stiDEBUG_TOOL
			}
			else
			{
				eResult = estiERROR;
				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					stiTrace("CstiVRCLServer::ClientAdd  There was an error "
					"getting the remote socket's IP address.\n");
				);
				eResult = estiERROR;
			}

			// Setup the socket to periodically test if the connection
			// is still alive
			nOptVal = 1;
			if (stiERROR == setsockopt (nSocket, SOL_SOCKET,
				SO_KEEPALIVE, (char *)&nOptVal, sizeof (nOptVal)))
			{

				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					stiTrace("CstiVRCLServer::ClientAdd  There was an error "
					"setting the SO_KEEPALIVE option on the socket.\n");
				);
				eResult = estiERROR;
			}


			// This call will tell the socket to close when close is
			// called even if there is still data to send.
			struct linger stLinger{};

			stLinger.l_onoff = 0;
			stLinger.l_linger = 0;

			if (stiERROR == setsockopt (nSocket, SOL_SOCKET,
				SO_LINGER, (char *)&stLinger, sizeof (stLinger)))
			{

				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					stiTrace("CstiVRCLServer::ClientAdd  There was an error "
					"setting the SO_LINGER option on the socket.\n");
				);
				eResult = estiERROR;
			}

			// Setup the socket options
			nOptVal = 0;

			// Tell the socket not to allow any delay when sending
			// packets.  This call will disable the Nagle algorithm
			nOptVal = 1;
			if (stiERROR == setsockopt (nSocket, IPPROTO_TCP,
				TCP_NODELAY, (char *)&nOptVal, sizeof (nOptVal)))
			{

				stiDEBUG_TOOL (g_stiVRCLServerDebug,
					stiTrace("CstiVRCLServer::ClientAdd  There was an error "
					"setting the TCP_NODELAY option on the socket.\n");
				);
				eResult = estiERROR;
			}

			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				stiTrace("CstiVRCLServer::ClientAdd:  A new client is "
				"connecting.\n");
			);

			// Keep track of the socket for this client connection
			m_nClientSocket = nSocket;

			// Monitor the client socket in the event loop so
			// we can read from it when data is available
			if (!FileDescriptorAttach (
				nSocket,
				[this,nSocket]{ eventReadClientData(nSocket); }))
			{
				stiASSERTMSG (estiFALSE, "%s: cant attach nSocket %d\n", __func__, nSocket);
			}

			// Inform the Application Layer of the connection
			connectionEstablishedSignal.Emit ();

			// time the client registration process
			m_connectionTimer.start ();
		}
	}

	return (eResult);
}


/*!
 * \brief Disconnects the client
 */
EstiResult CstiVRCLServer::ClientDisconnect ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::ClientDisconnect);

	stiDEBUG_TOOL (
		g_stiVRCLServerDebug,
		stiTrace ("CstiVRCLServer::ClientDisconnect: Disconnecting the client\n");
	);

	// Stop the life check timer, since we are no longer connected.
	m_lifeCheckTimer.stop ();

	// Stop the call in progress timer, since we are no longer connected.
	m_callInProgressTimer.stop ();

	// Do we have a client connected?
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Stop monitoring the client socket in the event loop
		// and close the socket
		FileDescriptorDetach (m_nClientSocket);
		stiOSClose (m_nClientSocket);
		m_nClientSocket = stiINVALID_SOCKET;
		m_bAuthenticated = estiFALSE;

		// Inform the application layer of the closure
		connectionClosedSignal.Emit ();
	}

	// Purge the circular buffer to clear out any old messages.
	m_oMessageBuffer.Purge ();

	return (estiOK);
}


/*!
 * \brief Removes XML-escaped chars and replaces with appropriate char
 *
 * \returns A pointer to the newly unescaped string or NULL in the case the
 *          buffer wasn't long enough to receive it.  In the error case, szLen
 *          will contain the length of the buffer (minus the NULL terminator)
 */
std::string strXunescape (
	const char *pszData)
{
	const unsigned int nMAX_ESCAPE_LEN = 6;
	char szChar[nMAX_ESCAPE_LEN];
	std::string result;
	unsigned int i = 0;
	unsigned int k;
	unsigned int l;

	//
	// Initialize the string to empty.
	//
	szChar[0] = '\0';

	while (pszData[i])
	{
		// Escape sequences are of the form "&aaa;" where the "&" is the
		// start of the sequence and ";" is the end and "aaa" is the representation
		// of the character that was escaped.
		if ('&' == pszData[i])
		{
			// We've found the beginning of a sequence.
			k = i; // Store the location of the beginning of the sequence
			i++;
			l = 0;
			while (pszData[i] != ';' &&  // Have we found the sequence terminator?
					pszData[i] &&         // Have we found the end of the string?
				   i < k + nMAX_ESCAPE_LEN) // Has the sequence exceeded in length all known sequences?
			{
				// No.  Get the character and increment the indexes
				szChar[l++] = pszData[i++];
			}

			// If we've found the terminator, convert the sequence to a valid character
			if (pszData[i] == ';')
			{
				if (0 == strncmp (szChar, "gt", 2))
				{
					result += '>';
				}
				else if (0 == strncmp (szChar, "lt", 2))
				{
					result += '<';
				}
				else if (0 == strncmp (szChar, "amp", 3))
				{
					result += '&';
				}
				else
				{
					// The escape sequence isn't understood, skip it
				}

				// Increment index i past the ";" now and continue
				i++;
			}
			else
			{
				// We've either failed to find the terminator or reached the end
				// of the string, copy the first character (the sequence start char)
				// into the buffer and reset the index to begin on the next character.
				i = k;
				result += pszData[i++];
			}
		}
		else
		{
			// Simply copy this character into the new buffer and increment
			// both of the indexes.
			result += pszData[i++];
		}
	}

	return result;
}


/*!
 * \brief Handles incoming client messages
 */
EstiResult CstiVRCLServer::ClientMessageProcess (
	char * byBuffer,
	int nBytesReadFromPort)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::ClientMessageProcess);

	EstiResult eResult = estiOK;
	stiHResult hResult;
	char *pszNodeType = nullptr;
	char *pszNodeType2 = nullptr;

	byBuffer[nBytesReadFromPort] = 0;

	if (nBytesReadFromPort > 0)
	{
#ifdef stiDEBUG
			char szXMLTagBuffer[nMAX_MESSAGE_BUFFER + 1] =
				"CstiVRCLServer::ClientMessageProcess Error.  XML Buffer is "
				"still in init state.  No data read.\n\0";
#else
			char szXMLTagBuffer[nMAX_MESSAGE_BUFFER + 1];
#endif

		auto  pszXMLTagBuffer = (char *)(&szXMLTagBuffer);
		char * pchData;

		uint32_t un32TagSize = 0;
		uint32_t un32DataSize = 0;
		EstiBool bTagFound = estiTRUE;
		EstiBool bInvalidTagFound = estiFALSE;
		EstiBool bHandled = estiFALSE;

		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("CstiVRCLServer::ClientMessageProcess - received %d bytes:"
				"\"%s\"\n", nBytesReadFromPort, byBuffer);
		);

		//
		// Write the incoming data to the circular buffer
		//
		uint32_t un32NumberOfBytesWritten = 0;

		m_oMessageBuffer.Write (
			byBuffer,
			nBytesReadFromPort,
			un32NumberOfBytesWritten);

		//
		// Read back tags from the circular buffer until there are no more
		//
		hResult = m_oMessageBuffer.GetNextXMLTag (
			pszXMLTagBuffer,
			nMAX_MESSAGE_BUFFER,
			un32TagSize,
			bTagFound,
			&pchData,
			un32DataSize,
			bInvalidTagFound);
		stiTESTRESULT();
		
		// If the tag buffer is empty, do not process
		stiTESTCOND_NOLOG (pszXMLTagBuffer && pszXMLTagBuffer[0], stiRESULT_SUCCESS);

		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			if (!bTagFound)
			{
				stiTrace ("CstiVRCLServer::ClientMessageProcess - \"%s\" Tag NOT FOUND!\n", pszXMLTagBuffer);
		    }
		);
		stiTESTCOND(bTagFound, stiRESULT_ERROR);

		// Keep processing as long as there are tags to process
		while (bTagFound &&
			   (eResult != estiERROR))
		{
			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				stiTrace ("CstiVRCLServer::ClientMessageProcess - Next Tag is: "
					"\"%s\" \n", pszXMLTagBuffer);
			);

			//
			// Skip past any whitespace at the beginning.
			//
			for (; *pszXMLTagBuffer != '\0' && isspace (*pszXMLTagBuffer); ++pszXMLTagBuffer)
			{
			}

			hResult = CreateXmlDocumentFromString(pszXMLTagBuffer, &m_pxDoc);

			if (hResult == stiRESULT_SUCCESS &&
				m_pxDoc)
			{
				hResult = GetChildNodeType((IXML_Node *)m_pxDoc,
										   &pszNodeType,
										   &m_pRootNode);
			}

			if (pszNodeType)
			{
				// Assume that the message is handled.  If it is not it will be reset to false.
				bHandled = estiTRUE;

				//
				// Skip to Signmail
				//
				if (0 == strcmp (pszNodeType, g_szSkipToSignMailRecord))
				{
					/* The XML for this will look like ...
					 * <SkipToSignMailRecord />
					 */
					
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
										stiTrace ("CstiVRCLServer::ClientMessageProcess> Received %s\n", g_szSkipToSignMailRecord);
					);
					
					// Get the first call object.
					auto call = m_pConferenceManager->CallObjectGet (
					            esmiCS_IDLE | esmiCS_CONNECTING | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH |
					            esmiCS_DISCONNECTING | esmiCS_DISCONNECTED | esmiCS_INIT_TRANSFER | esmiCS_TRANSFERRING);
					if (call)
					{
						call->SignMailSkipToRecord();
					}

					eResult = estiOK;
				}
				
				// 
				// Enable SignMail Recording
				//
				else if (0 == strcmp (pszNodeType, g_szEnableSignMailRecording))
				{
					std::string value;
					
					hResult = GetTextNodeValue (m_pRootNode, value);
					stiTESTRESULT ();
					
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						vpe::trace ("CstiVRCLServer::ClientMessageProcess Received ", g_szEnableSignMailRecording, "[", value, "]\n");
					);
					
					if (0 == stricmp (value.c_str (), g_szTrue))
					{
						m_bSignMailEnabled = estiTRUE;
						
						// Tell the client that we have succeeded in enabling the engine from TerpNet.
						char szBuf[255];
						snprintf (szBuf, sizeof (szBuf) - 1, "<%s />", g_szSignMailRecordingEnabled);
						eResult = ClientResponseSend (szBuf);
					}
					else
					{
						m_bSignMailEnabled = estiFALSE;
					}
				}
				
				//
				// TextBackspaceSend
				//
				else if (0 == strcmp (pszNodeType, g_szTextBackspaceSend))
				{
					hResult = TextBackspaceSend ();

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				//
				// TextMessageSend
				//
				else if (0 == strcmp (pszNodeType, g_szTextMessageSend))
				{
					hResult = TextMessageSend (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}
				
				//
				// TextMessageClear
				//
				else if (0 == strcmp (pszNodeType, g_szTextMessageClear))
				{
					hResult = TextMessageClear (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}
				
				//
				// TerpNetModeSet
				//
				else if (0 == strcmp (pszNodeType, g_szTerpNetModeSet))
				{
					hResult = TerpNetModeSet (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				//
				// SipStackStatsGet
				//
				else if (0 == strcmp (pszNodeType, g_szDiagnosticsGet))
				{
					// Call CCI to handle the request
					diagnosticsGetSignal.Emit ();
				}
				else if (0 == strcmp(pszNodeType, g_szExternalConferenceDialMessage))
				{
					hResult = externalConferenceDial (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						char szMessage[nMAX_MESSAGE_BUFFER];
						sprintf (szMessage,
								 "%s%s%s",
								 g_szCallTerminated,
								 g_szCallResultUnknown,
								 g_szCallTerminatedEnd);

						eResult = ClientResponseSend (szMessage);
						if (estiOK != eResult)
						{
							// No! Disconnect them now.
							ClientDisconnect ();
						}

						// Return back a success so that we don't disconnect and reconnect.
						eResult = estiOK;
					}
				}
				//
				// PhoneNumberDial
				//
				else if (0 == strcmp (pszNodeType, g_szPhoneNumberDialMessage))
				{
					hResult = PhoneNumberDial (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						char szMessage[nMAX_MESSAGE_BUFFER];
						sprintf (szMessage,
								 "%s%s%s",
								 g_szCallTerminated,
								 g_szCallResultUnknown,
								 g_szCallTerminatedEnd);

						eResult = ClientResponseSend (szMessage);
						if (estiOK != eResult)
						{
							// No! Disconnect them now.
							ClientDisconnect ();
						}

						// Return back a success so that we don't disconnect and reconnect.
						eResult = estiOK;
					}
				}

				//
				// LoggedInPhoneNumberGet
				//
				else if (0 == strcmp (pszNodeType, g_szLoggedInPhoneNumberGet))
				{
					// Looks like this:
					/* <LoggedInPhoneNumberGet />
					 */
					
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szLoggedInPhoneNumberGet);
					);
					
					// Send the LoggedInPhoneNumber
					char szResponse[256];
					const char *pszLoggedInPN = (m_strLoggedInPhoneNumber.empty()) ? "" : m_strLoggedInPhoneNumber.c_str();

					// Put the LoggedInPhoneNumber information into a string that can be
					// returned.
					sprintf (szResponse, "<%s>%s</%s>",
						g_szLoggedInPhoneNumber,
						pszLoggedInPN,
						g_szLoggedInPhoneNumber);

					// Write the response to the client
					eResult = ClientResponseSend (szResponse);
				}

				//
				// UpdateCheck
				//
				else if (0 == strcmp (pszNodeType, g_szUpdateCheck))
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
										stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szUpdateCheck);
					);

					hResult = UpdateCheck (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				//
				// CallIdSet
				//
				else if (0 == strcmp (pszNodeType, g_szCallIdSet))
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
										stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szCallIdSet);
					);

					hResult = CallIdSet (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				//
				// AllowedAudioCodecs
				//
				else if (0 == strcmp (pszNodeType, g_szAllowedAudioCodecs))
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
										stiTrace ("CstiVRCLServer::AllowedAudioCodecs Received %s\n", g_szAllowedAudioCodecs);
					);
					
					hResult = AllowedAudioCodecs (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				// 
				// Clear the Announce text message from the UI
				//
				else if (0 == strcmp (pszNodeType, g_szAnnounceClear))
				{
					/* Looks like this:
					 * <AnnounceClear />
					 */
					
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szAnnounceClear);
					);
					
					// Notify the CCI to clear the Announce text message
					vrsAnnounceClearSignal.Emit ();
					
					eResult = estiOK;
				}
				

				// 
				// Set the Announce text message on the UI
				//
				else if (0 == strcmp (pszNodeType, g_szAnnounceSet))
				{
					std::string value;

					hResult = GetTextNodeValue (m_pRootNode, value);
					stiTESTRESULT ();

					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						vpe::trace ("CstiVRCLServer::ClientMessageProcess Received ", g_szAnnounceSet, "[", value, "]\n");
					);

					// Notify the CCI to set Announce text message.
					vrsAnnounceSetSignal.Emit (value);
				}

				//
				// Send DTMF
				//
				else if (0 == strcmp (pszNodeType, g_szDtmfSend))
				{
					/* Looks like this:
					 * <DtmfSend>n</DtmfSend>
					 */
					std::string value;

					hResult = GetTextNodeValue (m_pRootNode, value);
					stiTESTRESULT ();

					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						vpe::trace ("CstiVRCLServer::ClientMessageProcess Received ", g_szDtmfSend, "[", value, "]\n");
					);

					// Get the active call object
					auto call = callObjectGet(esmiCS_CONNECTED);

					if (call)
					{
						if (call->DtmfSupportedGet ())
						{
							// Do we have a valid DTMF digit to send?
							EstiDTMFDigit eDigit;
							bool bValid = true;

							switch (value[0])
							{
								case '0':
									eDigit =  estiDTMF_DIGIT_0;
									break;

								case '1':
									eDigit =  estiDTMF_DIGIT_1;
									break;

								case '2':
									eDigit =  estiDTMF_DIGIT_2;
									break;

								case '3':
									eDigit =  estiDTMF_DIGIT_3;
									break;

								case '4':
									eDigit =  estiDTMF_DIGIT_4;
									break;

								case '5':
									eDigit =  estiDTMF_DIGIT_5;
									break;

								case '6':
									eDigit =  estiDTMF_DIGIT_6;
									break;

								case '7':
									eDigit =  estiDTMF_DIGIT_7;
									break;

								case '8':
									eDigit =  estiDTMF_DIGIT_8;
									break;

								case '9':
									eDigit =  estiDTMF_DIGIT_9;
									break;

								case '*':
									eDigit =  estiDTMF_DIGIT_S;
									break;

								case '#':
									eDigit =  estiDTMF_DIGIT_P;
									break;

								default:
									bValid = false;
									break;
							}

							if (bValid)
							{
								call->DtmfToneSend (eDigit);
							}
						}
					}

					eResult = estiOK;
				}

				//
				// VRCL Client sending HearingStatus
				//
				else if (0 == strcmp (pszNodeType, g_szHearingStatus))
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
										stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szHearingStatus);
					);

					hResult = HearingStatus (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				//
				// VRCL Client sending RelayNewCallReady
				//
				else if (0 == strcmp (pszNodeType, g_szRelayNewCallReady))
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
										stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szRelayNewCallReady);
					);

					hResult = RelayNewCallReady (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				//
				// Text in SignMail
				//
				else if (0 == strcmp (pszNodeType, g_szTextInSignMail))
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szTextInSignMail);
					);

					hResult = textInSignMailSet (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				else if (0 == strcmp(pszNodeType, g_hearingVideoCapability))
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::ClientMessageProcess Received %s\n", g_hearingVideoCapability);
					);

					hResult = hearingVideoCapability (m_pRootNode);

					if (stiIS_ERROR (hResult))
					{
						eResult = estiERROR;
					}
				}

				else if (0 == strcmp(pszNodeType, g_szExternalCameraUseMessage))
				{
					stiDEBUG_TOOL(g_stiVRCLServerDebug,
						stiTrace("CstiVRCLServer::ClientMessageProcess Received %s\n", g_szExternalCameraUseMessage);
					);

					hResult = externalCameraUse(m_pRootNode);

					if (stiIS_ERROR(hResult))
					{
						eResult = estiERROR;
					}
				}
				else
				{
					stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer Unhandled message %s\n", pszNodeType);
					);
					// Since it wasn't handled, set the boolean to indicate such.
					bHandled = estiFALSE;
				}
			}

			// Cleanup the XML variables, so we don't have any leaks.
			if (pszNodeType)
			{
				// Free up memory allocated by GetChildNodeType
				delete [] pszNodeType;
				pszNodeType = nullptr;
			}

			if (pszNodeType2)
			{
				// Free up memory allocated by GetChildNodeType
				delete [] pszNodeType2;
				pszNodeType2 = nullptr;
			}

			// Cleanup the XML doc if we created one.
			if (m_pxDoc)
			{
				CleanupXmlDocument(m_pxDoc);
				m_pxDoc = nullptr;
				m_pRootNode = nullptr;
			}

			if (!bHandled)
			{
				//
				// Connect
				//
				if (0 == strncmp (
					pszXMLTagBuffer,
					g_szConnectMessage,
					strlen (g_szConnectMessage)))
				{
					// Null Terminate the key value
					pchData[un32DataSize] = 0;

					// Reinitialize the last key
					m_szLastServerKey[0] = '\0';

					// Authenticate the key
#ifdef IPV6_ENABLED
					int nSockaddrSize = sizeof (sockaddr_in6);
					struct sockaddr *pConnectedSocketAddress = (sockaddr *)new sockaddr_in6;
#else
					int nSockaddrSize = sizeof (sockaddr_in);
					auto pConnectedSocketAddress = (sockaddr *)new sockaddr_in;
#endif
					getpeername (m_nClientSocket, pConnectedSocketAddress, (socklen_t *)&nSockaddrSize);
					
//Android has problems trying to use ifaddrs so we need to use the old method.
#if APPLICATION != APP_NTOUCH_ANDROID
					if (pConnectedSocketAddress->sa_family == AF_INET)
					{
						// Retreive the local IP address from the bootline.
						std::string LocalIP;
						stiGetLocalIp (&LocalIP, estiTYPE_IPV4);
						uint32_t anLocalIp[1];
						inet_pton (pConnectedSocketAddress->sa_family, LocalIP.c_str (), &anLocalIp);
						// Validate the key data using the traditional method. Does it
						// validate?
						eResult = stiClientValidate (
							pConnectedSocketAddress->sa_family,
							(uint32_t*)&((sockaddr_in*)pConnectedSocketAddress)->sin_addr,
							anLocalIp,
							pchData);

					}
#ifdef IPV6_ENABLED
					else if (pConnectedSocketAddress->sa_family == AF_INET6)
					{
						// Retreive the local IP address from the bootline.
						std::string LocalIP;
						stiGetLocalIp (&LocalIP, estiTYPE_IPV6);
						uint32_t anLocalIp[4];
						inet_pton (pConnectedSocketAddress->sa_family, LocalIP.c_str (), anLocalIp);
						
						// Validate the key data using the traditional method. Does it
						// validate?
						eResult = stiClientValidate (
							pConnectedSocketAddress->sa_family,
							(uint32_t*)&((sockaddr_in6*)pConnectedSocketAddress)->sin6_addr,
							anLocalIp,
							pchData);
						
					}
#endif
					else
					{
						eResult = estiERROR;
					}
#else
					if (pConnectedSocketAddress->sa_family == AF_INET)
					{
						// Retreive the local IP address from the bootline.
						std::string IPAddress;
						stiGetLocalIp (&IPAddress, estiTYPE_IPV4);

						eResult = stiClientValidate (
												m_un32ClientIPAddress,
												(uint32_t) htonl(inet_addr (IPAddress.c_str ())),
												pchData);
					}
#endif
					delete pConnectedSocketAddress;
					pConnectedSocketAddress = nullptr;

					if (estiOK == eResult)
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess "
								"Received Valid Connect Message! \n");
						);

						// Yes! Stop the connection timer, since we are now connected.
						m_connectionTimer.stop ();

						// Send a response to the Client that they have been
						// accepted

						// Can we write the response to the client?
						eResult = ClientResponseSend (g_szConnectAck);

						// Keep track that we have authenticated successfully
						m_bAuthenticated = estiTRUE;
					}
					else
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess "
								"Trying new connection method. \n");
						);

						// No! The traditional method did not produce a match, so
						// try the new method of connecting.

						// Store the seed that was sent.
						strncpy (m_szLastClientKey, pchData, sizeof (m_szLastClientKey));
						m_szLastClientKey [sizeof(m_szLastClientKey) - 1] = 0;

						// Respond with a random Key.
						stiKeyCreate (stiOSTickGet (), m_szLastServerKey);

						char szMessage[nMAX_MESSAGE_BUFFER];
						sprintf (
							szMessage,
							"%s%s%s",
							g_szConnectAckBegin,
							m_szLastServerKey,
							g_szConnectAckEnd);

						// Write the response to the client?
						eResult = ClientResponseSend (szMessage);
					}
				}

				//
				// Validate
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szValidateMessage,
					strlen (g_szValidateMessage)))
				{
					// Null Terminate the password value
					pchData[un32DataSize] = 0;

					// Do we still have the last key?
					if (m_szLastServerKey[0] != '\0')
					{
						// Yes! Validate the password. Does it validate?
						eResult = stiPasswordValidate (
							m_szLastClientKey,
							m_szLastServerKey,
							pchData);
						if (estiOK == eResult)
						{
							stiDEBUG_TOOL (g_stiVRCLServerDebug,
								stiTrace ("CstiVRCLServer::ClientMessageProcess "
									"Received Valid Validate Message! \n");
							);

							// Yes! Stop the connection timer, since we are now connected.
							m_connectionTimer.stop ();

							// Send a response to the Client that they have been
							// accepted

							// Write the response to the client
							eResult = ClientResponseSend (g_szValidateAck);

							// Keep track that we have authenticated successfully
							m_bAuthenticated = estiTRUE;
						}
					}
				}

				else if (m_bAuthenticated)
				{
					//
					// API Version Get
					//
					if (0 == strncmp (
						pszXMLTagBuffer,
						g_szAPIVersionGetMessage,
						strlen (g_szAPIVersionGetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"API Version GetMessage! \n");
						);

						// Grab the API version sent by the client.
						char szVersion[nMAX_MESSAGE_BUFFER];
						strncpy (szVersion, pchData, un32DataSize);
						szVersion[un32DataSize] = 0;

						// Store the API version sent by the client.
						sscanf (
							szVersion,
							"%d.%d",
							&m_nClientAPIMajorVersion,
							&m_nClientAPIMinorVersion);

						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess - "
								"Detected API version %d.%d\n",
								m_nClientAPIMajorVersion,
								m_nClientAPIMinorVersion);
						);

						// Send the API version
						ApiVersionSend ();

						m_lifeCheckTimer.restart ();
					}

					//
					// Application Version Get
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szApplicationVersionGetMessage,
						strlen (g_szApplicationVersionGetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Application Version GetMessage! \n");
						);

						// Send the App version
						ApplicationVersionSend ();
					}
					//
					// Application Capabilities Get
					//
					else if (0 == strncmp(
						pszXMLTagBuffer,
						g_szApplicationCapabilitiesGetMessage,
						strlen(g_szApplicationCapabilitiesGetMessage)))
					{
						stiDEBUG_TOOL(g_stiVRCLServerDebug,
							stiTrace("CstiVRCLServer::ClientMessageProcess Received "
								"Application Capabilities GetMessage! \n");
						);

						// Send the App version
						ApplicationCapabilitiesSend();
					}

					//
					// Answer
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szCallAnswerMessage,
						strlen (g_szCallAnswerMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Answer Message! \n");
						);

						// Get the incoming call object
						auto call = m_pConferenceManager->CallObjectIncomingGet ();
						if (call)
						{
							call->Answer ();
						}
					}

					//
					// Reject
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szCallRejectMessage,
						strlen (g_szCallRejectMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Reject Message! \n");
						);

						// Get the incoming call object
						auto call = m_pConferenceManager->CallObjectIncomingGet ();
						if (call)
						{
							call->Reject (estiRESULT_LOCAL_SYSTEM_REJECTED);
						}
					}

					//
					// Hang Up
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szCallHangupMessage,
						strlen (g_szCallHangupMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Hang Up Message!\n");
						);

						// Is there an "incoming" call?
						auto call = m_pConferenceManager->CallObjectIncomingGet ();

						// If there is an incoming call treat this
						// as a "Reject" instead of as a "Hang-up".  The VP Control
						// application doesn't provide a "Reject" button.
						if (call)
						{
							call->Reject (estiRESULT_LOCAL_SYSTEM_REJECTED);
						}
						else
						{
							auto vrclCall = callObjectGet(esmiCS_CONNECTING | esmiCS_CONNECTED | esmiCS_HOLD_LCL | esmiCS_HOLD_RMT | esmiCS_HOLD_BOTH);
							if (nullptr != vrclCall)
							{
								vrclCall->HangUp();
							}
							else
							{
								// Get a copy of the list of calls.
								// Loop on this list and for each call in the Disconnected state with either
								// the LeaveMessage or MessageComplete substate, cause a message to be sent
								// up to do a signmail hangup.
								std::list<CstiCallSP> callList;
								m_pConferenceManager->CallObjectListGet(&callList);

								for (auto callx : callList)
								{
									if (callx->SignMailCompletedGet())
									{
										// Notify CCI that we need to hang up the call.
										signMailHangupSignal.Emit();
										break;
									}
								}
							}
						}
					}

					//
					// AddressDial
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szAddressDialMessage,
						strlen (g_szAddressDialMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Address Dial Message! \n");
						);
						SstiCallDialInfo CallDialInfo;
						
						CallDialInfo.eDialMethod = estiDM_BY_DIAL_STRING;
						CallDialInfo.DestNumber.assign(pchData, un32DataSize);
						CallDialInfo.HearingNumber = m_szHearingPhoneNumber;

						callDialSignal.Emit (CallDialInfo);

						// Blank out the hearing number now since it will be stored in the
						// call object at this point.
						m_szHearingPhoneNumber[0] = '\0';
					}
					else if (0 == strncmp(
						pszXMLTagBuffer,
						g_szCallBridgeDialMessage,
						strlen (g_szCallBridgeDialMessage)))
					{
						stiDEBUG_TOOL(g_stiVRCLServerDebug,
							stiTrace ( "CstiVRCLServer::ClientMessageProcess Received"
											"CallBridge Dial Message.\n");
						);

						// Check call bridge support
						hResult = stiMAKE_ERROR (stiRESULT_ERROR);
						int nEnableCallBridge = 0;
						m_poPM->propertyGet (ENABLE_CALL_BRIDGE, &nEnableCallBridge);

						if (nEnableCallBridge)
						{
							// Get the active call object
							auto call = callObjectGet (esmiCS_CONNECTED);
							if (call)
							{
								std::string buf;
								if (pchData)
								{
									buf.assign (pchData, un32DataSize);
								}

								hResult = call->BridgeCreate (buf);
								if (!stiIS_ERROR (hResult))
								{
									call->BridgeCompletionNotifiedSet (false);
								}
							}
						}

						// TODO do we need more than one block of code for the error condition any longer?
						if (stiIS_ERROR (hResult))
						{
							stiASSERTMSG(false, "Error Creating Bridge Enabled=%d", nEnableCallBridge);
							// Tell the client that we did not make the transfer
							char szMessage [256];
							sprintf (szMessage, "%s%s%s", g_szCallBridgeFail, nEnableCallBridge ? g_szNoActiveCall : g_szCallBridgeDisabled, g_szCallBridgeFailEnd);
							eResult = ClientResponseSend (szMessage);
						}

					}

					else if (0 == strncmp(
						pszXMLTagBuffer,
						g_szCallBridgeDisconnect,
						strlen(g_szCallBridgeDisconnect)))
					{
						// Get the active call object
						auto call = m_pConferenceManager->CallObjectGet (esmiCS_CONNECTED);
						hResult = stiMAKE_ERROR(stiRESULT_ERROR);
						if (call)
						{
							// Set the bridge completion notifcation so that when it is shutdown
							// the client is not notified that it is an error.
							call->BridgeCompletionNotifiedSet (true);

							hResult = call->BridgeDisconnect();

							// Tell the client that we have succeeded
							if (!stiIS_ERROR (hResult)) {
								eResult = ClientResponseSend (g_szCallBridgeDisconnectSuccess);
							}
							else
							{
								eResult = ClientResponseSend (g_szCallBridgeDisconnectFail);
							}
						}
					}

					else if (0 == strncmp(
							pszXMLTagBuffer,
							g_szEnableCallBridge,
							strlen(g_szEnableCallBridge)))
					{
						int nEnableCallBridge = 0;
						m_poPM->propertyGet (ENABLE_CALL_BRIDGE, &nEnableCallBridge);

						if (nEnableCallBridge)
						{
							ClientResponseSend (g_szCallBridgeEnabled);
						}
						else
						{
							ClientResponseSend (g_szCallBridgeDisabled);
						}
					}

					//
					// VRSDial
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szVRSDialMessage,
						strlen (g_szVRSDialMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"VRS Dial Message! \n");
						);

						SstiCallDialInfo CallDialInfo;

						CallDialInfo.eDialMethod = estiDM_BY_VRS_PHONE_NUMBER;
						CallDialInfo.DestNumber.assign(pchData, un32DataSize);

						callDialSignal.Emit (CallDialInfo);
					}

					//
					// VRSHearingPhoneSet
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szVRSHearingPhoneSetMessage,
						strlen (g_szVRSHearingPhoneSetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"VRS Hearing Phone Set Message! \n");
						);

						// Parse the hearing phone number, removing anything except
						// digits and the "x" character
						int nDest = 0;
						unsigned int nSrc = 0;
						for (;
							nSrc < un32DataSize && nSrc <= un8stiDIAL_STRING_LENGTH;
							nSrc++)
						{
							if (isdigit (pchData[nSrc]) || pchData[nSrc] == 'x')
							{
								//
								// Ensure that the first digit is a one.  This
								// banks on the idea that an area code does not
								// start with '1'.
								//
								if (nDest == 0 && pchData[nSrc] != '1')
								{
									m_szHearingPhoneNumber[nDest++] = '1';
								}

								m_szHearingPhoneNumber[nDest++] = pchData[nSrc];
							}
						}
						m_szHearingPhoneNumber[nDest] = 0;

						if (0 == strlen (m_szHearingPhoneNumber))
						{
							strcpy (m_szHearingPhoneNumber, g_szUNKNOWN);
						}
					}

					//
					// VRSVCODial
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szVRSVCODialMessage,
						strlen (g_szVRSVCODialMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"VRS VCO Dial Message! \n");
						);

						SstiCallDialInfo CallDialInfo;

						CallDialInfo.eDialMethod = estiDM_BY_VRS_WITH_VCO;
						CallDialInfo.DestNumber.assign(pchData, un32DataSize);

						callDialSignal.Emit (CallDialInfo);
					}

					//
					// CallTransfer
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szCallTransferMessage,
						strlen (g_szCallTransferMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Call Transfer Message! \n");
						);

						// Get the active call object
						auto call = m_pConferenceManager->CallObjectGet (esmiCS_CONNECTED);
						if (call)
						{
							std::string dialString {pchData, un32DataSize};

							stiHResult hResult = call->Transfer (dialString);

							if (stiIS_ERROR (hResult))
							{
								// Tell the client that we have Failed
								char szMessage [256];
								sprintf (szMessage, "%s%s%s", g_szCallTransferFail, g_szVideophoneError, g_szCallTransferFailEnd);
								eResult = ClientResponseSend (szMessage);
							}
							else
							{
								// Tell the client that we have succeeded
								eResult = ClientResponseSend (g_szCallTransferSuccess);
							}
						}
						else
						{
							// Tell the client that we did not make the transfer
							char szMessage [256];
							sprintf (szMessage, "%s%s%s", g_szCallTransferFail, g_szNoActiveCall, g_szCallTransferFailEnd);
							eResult = ClientResponseSend (szMessage);
						}
					}

					//
					// Bandwidth Detection Begin
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szCallBandwidthDetectionBegin,
						strlen (g_szCallBandwidthDetectionBegin)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Bandwidth Detect Message! \n");
						);

						// Get the active call object
						auto call = callObjectGet (esmiCS_CONNECTED);
						if (call)
						{
							stiHResult hResult = call->BandwidthDetectionBegin ();

							if (stiIS_ERROR (hResult))
							{
								// Tell the client that we have Failed
								eResult = ClientResponseSend (
									g_szCallBandwidthDetectionBeginFail);
							}
							else
							{
								// Tell the client that we have succeeded
								eResult = ClientResponseSend (
									g_szCallBandwidthDetectionBeginSuccess);
							}
						}
					}


					//
					// IsAlive
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szIsAliveMessage,
						strlen (g_szIsAliveMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"IsAlive Message! \n");
						);

						// Send a response to the Client that they have been
						// heard

						// Write the response to the client
						eResult = ClientResponseSend (g_szIsAliveAck);
					}


					//
					// IsAliveAck
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szIsAliveAckPartial,
						strlen (g_szIsAliveAckPartial)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"IsAliveAck Message! \n");
						);

						// Decrement the pending requests
						m_unPendingLifeCheckResponses--;
					}

					//
					// IsCallTransferable
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szIsCallTransferableMessage,
						strlen (g_szIsCallTransferableMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"IsCallTransferable Message! \n");
						);
						// Send whether or not this call is transferable.
						CallIsTransferableResponseSend ();
					}

					//
					// LocalNameSet
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szLocalNameSetMessage,
						strlen (g_szLocalNameSetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Local Name Set Message! \n");
						);

						// Null-terminate the data that was received
						pchData[un32DataSize] = 0;

						auto unescaped = strXunescape (pchData);

						localNameSetSignal.Emit (unescaped);

						// Write the response to the client
						ClientResponseSend (g_szLocalNameSetSuccess);
					}

					//
					// Device Name Get
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szDeviceTypeGetMessage,
						strlen (g_szDeviceTypeGetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Device Name Get Message! \n");
						);
						// Build the response
						char szResponse[256];

						// Put the information into a string that can be returned.
						sprintf (szResponse,
#if APPLICATION == APP_NTOUCH_IOS
							"%sNTOUCH-IOS%s",
#elif APPLICATION == APP_NTOUCH_MAC
							"%sNTOUCH-MAC%s",
#elif APPLICATION == APP_NTOUCH_PC
							"%sNTOUCH-PC%s",
#elif APPLICATION == APP_NTOUCH_VP3
							"%sVP-500%s",	
#elif APPLICATION == APP_NTOUCH_VP4
							"%sVP-600%s",	
#elif APPLICATION == APP_NTOUCH_VP2
							"%sVP-400%s",
#else
							"%sVP200%s",
#endif
							g_szDeviceTypeMessage,
							g_szDeviceTypeMessageEnd);

						// Can we write the response to the client?
						EstiResult eResult = ClientResponseSend (szResponse);
						if (estiOK != eResult)
						{
							// No! Disconnect them now.
							ClientDisconnect ();
						}
					}

					//
					// StatusCheck
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szStatusCheckMessage,
						strlen (g_szStatusCheckMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"StatusCheck Message! \n");
						);

						// Send the state
						eResult = StateSend ();
					}

					//
					// Reboot
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szRebootMessage,
						strlen (g_szRebootMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"Reboot Message! \n");
						);

						// Disconnect the client, so they aren't wondering what happened
						// after we reboot.
						ClientDisconnect ();

						PropertyManager::getInstance()->saveToPersistentStorage();

						// Delay 1 sec. to ensure persistent data is written to flash.
						stiOSTaskDelay (1000);

						// Reboot the machine.
						rebootDeviceSignal.Emit ();
					}

					//
					// SettingGet
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szSettingGetMessage,
						strlen (g_szSettingGetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"SettingGet Message! \n");
						);

						// Write the response to the client
						eResult = SettingSend (
							// NOTE: We are sending the message attributes here, NOT the data!
							(pszXMLTagBuffer + sizeof (g_szSettingGetMessage)));
					}

					//
					// SettingSet
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szSettingSetMessage,
						strlen (g_szSettingSetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
								"SettingSet Message! \n");
							stiTrace ("\tTag Buffer = %s\n\tData = %s, DataSize = %d\n",
								pszXMLTagBuffer + sizeof (g_szSettingSetMessage), pchData, un32DataSize);
						);

						// Change the specified setting. Did it change?
						if (SettingChange (
							(pszXMLTagBuffer + sizeof (g_szSettingSetMessage)), // attributes
							pchData, // data
							un32DataSize) // data length
							== estiOK)
						{
							// Yes! Tell the client that we have succeeded
							eResult = ClientResponseSend (g_szSettingSetSuccess);
						}
						else
						{
							// No! Tell the client that we have Failed
							eResult = ClientResponseSend (g_szSettingSetFail);
						}
					}
					//
					// Call Hold Set
					//
					else if (0 == strncmp(
					pszXMLTagBuffer,
					g_szCallHoldSetMessage,
					strlen(g_szCallHoldSetMessage)))
					{
					stiDEBUG_TOOL(g_stiVRCLServerDebug,
						stiTrace("CstiVRCLServer::ClientMessageProcess Received "
							"Call Hold Set Message! \n");
					);

					bool hold = (0 == strncmp(
						pchData,
						g_szOnSwitch,
						strlen(g_szOnSwitch)));

					// Get the active call object
					auto call = m_pConferenceManager->CallObjectHeadGet();
					if (call)
					{
						if (hold)
						{
							eResult = ClientResponseSend(stiIS_ERROR(call->Hold()) ? g_szCallHoldSetFail : g_szCallHoldSetSuccess);
						}
						else
						{
							eResult = ClientResponseSend(stiIS_ERROR(call->Resume()) ? g_szCallHoldSetFail : g_szCallHoldSetSuccess);
						}
					}
					else
					{
						eResult = ClientResponseSend(g_szCallHoldSetFail);
					}
					}
					//
					// Video Privacy Set
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szVideoPrivacySetMessage,
						strlen (g_szVideoPrivacySetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
									"Video Privacy Set Message! \n");
						);

						EstiSwitch eSwitch = estiOFF;

						if (0 == strncmp (
							pchData,
							g_szOnSwitch,
							strlen (g_szOnSwitch)))
						{
							eSwitch = estiON;
						}

						stiHResult hResult = IstiVideoInput::InstanceGet ()->PrivacySet (eSwitch == estiON ? estiTRUE : estiFALSE);

						if (stiIS_ERROR (hResult))
						{
							// Tell the client that we have Failed
							eResult = ClientResponseSend (
									g_szVideoPrivacySetFail);
						}
						else
						{
							// Tell the client that we have succeeded
							eResult = ClientResponseSend (
									g_szVideoPrivacySetSuccess);
						}
					}

					//
					// Audio Privacy Set
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szAudioPrivacySetMessage,
						strlen (g_szAudioPrivacySetMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
									"Audio Privacy Set Message! \n");
						);

						EstiSwitch eSwitch = estiOFF;

						if (0 == strncmp (
							pchData,
							g_szOnSwitch,
							strlen (g_szOnSwitch)))
						{
							eSwitch = estiON;
						}

						stiHResult hResult = IstiAudioInput::InstanceGet ()->PrivacySet (eSwitch == estiON ? estiTRUE : estiFALSE);

						if (stiIS_ERROR (hResult))
						{
							// Tell the client that we have Failed
							eResult = ClientResponseSend (
									g_szAudioPrivacySetFail);
						}
						else
						{
							// Tell the client that we have succeeded
							eResult = ClientResponseSend (
									g_szAudioPrivacySetSuccess);
						}
					}

					//
					// In Call Stats Get
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szInCallStatsGet,
						strlen (g_szInCallStatsGet)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
									"In Call Stats Check Message! \n");
						);
						assert(false);
						// Get the active call object
						auto call = callObjectGet (esmiCS_CONNECTED);
						InCallStatsSend (call);

						eResult = estiOK;
					}

#if 0
					//
					// Print Screen
					//
					else if (0 == strncmp (
						pszXMLTagBuffer,
						g_szPrintScreenMessage,
						strlen (g_szPrintScreenMessage)))
					{
						stiDEBUG_TOOL (g_stiVRCLServerDebug,
							stiTrace ("CstiVRCLServer::ClientMessageProcess Received "
									"Print Screen Message! \n");
						);

	//		 			m_pConference->PrintScreen (); // Send an async message to app via CCI.

						eResult = estiOK;
					} // end if
#endif
			   }
			} // !handled

			// Try to grab a new tag to process
			pszXMLTagBuffer = szXMLTagBuffer;

			hResult = m_oMessageBuffer.GetNextXMLTag (
				pszXMLTagBuffer,
				nMAX_MESSAGE_BUFFER,
				un32TagSize,
				bTagFound,
				&pchData,
				un32DataSize,
				bInvalidTagFound);
			stiTESTRESULT();

		} // End while loop

		//
		// Process any properties that we received.
		// The reason this is here is so that we can minimize the number of times
		// we call the settings methods of subsystems due to property changes. This will work
		// as long as the VRCL client sends us bundles of properties to set.
		//
		if (!m_PropertyList.empty ())
		{
			propertiesSetSignal.Emit (m_PropertyList);
			m_PropertyList.clear ();
		}

	} // end if

STI_BAIL:

	if (pszNodeType)
	{
		// Free up memory allocated by GetChildNodeType
		delete [] pszNodeType;
		pszNodeType = nullptr;
	}

	if (pszNodeType2)
	{
		// Free up memory allocated by GetChildNodeType
		delete [] pszNodeType2;
		pszNodeType2 = nullptr;
	}

	// Cleanup the XML doc if we created one.
	if (m_pxDoc)
	{
		CleanupXmlDocument(m_pxDoc);
		m_pxDoc = nullptr;
		m_pRootNode = nullptr;
	}

	return (eResult);
} // end CstiVRCLServer::ClientMessageProcess


/*!
 * \brief Sends response to client application
 */
EstiResult CstiVRCLServer::ClientResponseSend (
    const char *szMessage)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::ClientResponseSend);

	EstiResult eResult = estiOK;

	stiDEBUG_TOOL (g_stiVRCLServerDebug,
		stiTrace ("CstiVRCLServer::ClientResponseSend - "
			"Sending: %s\n", szMessage);
	);

	// Send response to the Client
	int nBytesToWrite = strlen (szMessage);
	int nBytesWritten = stiOSWrite (
		m_nClientSocket,
		szMessage,
		nBytesToWrite);

	// Was there an error writing to the client?
	if (nBytesToWrite != nBytesWritten)
	{
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("CstiVRCLServer::ClientResponseSend "
				"Error Writing Response! \n");
		);

		// Yes! Return an error
		stiASSERT (estiFALSE);
		eResult = estiERROR;
	}

	return (eResult);
}


/*!
 * \brief Sends response to client application
 */
EstiResult CstiVRCLServer::CallIsTransferableResponseSend ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::CallIsTransferableResponseSend);

	const char* pszMsg = g_szCallIsNotTransferable;

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// If the engine is in the "Ready" state and we have
		// only a single call object
		if (m_pConferenceManager->eventLoopIsRunning ()
		&& 1 == m_pConferenceManager->CallObjectsCountGet ())
		{
			//TODO implement state validate for External calls
			//auto call = callObjectHeadGet ();
			auto call = m_pConferenceManager->CallObjectHeadGet ();

			// The call IS transferable if:
			//
			// The call must not be "connected" (not on hold)
			// The call must have been negotiated as "transferable" OR
			// The call must have been originated by the opposite endpoint
			if (call)
			{
				//Is the call in a connected state?
				if (call->StateValidate (esmiCS_CONNECTED) &&

					// Did the call get negotiated as "transferable" OR
					// Is the call an "incoming" call (originated remotely)?
				    (call->IsTransferableGet () ||
				    estiINCOMING == call->DirectionGet ()))
				{
					pszMsg = g_szCallIsTransferable;
				}
			}
		}
	}


	// Can we write the response to the client?
	EstiResult eResult = ClientResponseSend (pszMsg);
	if (estiOK != eResult)
	{
		// No! Disconnect them now.
		ClientDisconnect ();
	}

	return (estiOK);
}


/*!
 * \brief Sends info about the connected state to the client
 */
EstiResult CstiVRCLServer::ConnectedStateSend (
    VRCLCallSP call)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::ConnectedStateSend);
	
	EstiResult eResult = estiERROR;

	if (call)
	{
		// We are connected. Create a message to the client
		// indicating with whom we are connected.
		// Send a full message to the Client with our status
		std::stringstream Message;

		// Add the beginning tag...
		Message << g_szStatusInCallEx;

		// Add the user's data
		UserInfoAdd (call, &Message);

		// Add the ending tag
		Message << g_szStatusInCallExEnd;

		// Send the message to the client.
		eResult = ClientResponseSend (Message.str ().c_str ());
	}

	return (eResult);
}


/*!
 * \brief Reads a settings from PD and sends it to the client
 */
EstiResult CstiVRCLServer::SettingSend (
	char * szData)
{
	EstiResult eReturn = estiERROR;
	char szType[24];
	char szName[48];
	char szMessage[nMAX_MESSAGE_BUFFER];
	char szTmp[nMAX_MESSAGE_BUFFER];
	int nTmp = nMAX_MESSAGE_BUFFER;
	int nValue = 0;

	stiLOG_ENTRY_NAME (CstiVRCLServer::SettingSend);

	// Parse the type and item name
	if (2 == sscanf (szData, "Type=\"%23s Name=\"%47s", szType, szName))
	{
		szType[strlen (szType) -1] = '\0';
		szName[strlen (szName) -1] = '\0';
		if (0 == strcmp (szType, "Number"))
		{
			m_poPM->propertyGet (szName, &nValue);
			sprintf (
				szMessage,
				g_szNumberSetting,
				szType,
				szName,
				nValue);

			// Send the message
			eReturn = ClientResponseSend (szMessage);
		}
		else if (0 == strcmp (szType, "String"))
		{
			std::string value;

			m_poPM->propertyGet (szName, &value);
			sprintf (
				szMessage,
				g_szStringSetting,
				szType,
				szName,
				strXescape (value.c_str (), szTmp, &nTmp));

			// Send the message
			eReturn = ClientResponseSend (szMessage);
		}
		else if (0 == strcmp (szType, "Enum"))
		{
			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				stiTrace ("<VRCLServer::SettingSend> Request for type \"enum\" is not yet implemented.\n");
			);
			eReturn = ClientResponseSend (g_szSettingGetFail);
		}
		else
		{
			// The property type is unknown.  Send a failure response.
			eReturn = ClientResponseSend (g_szSettingGetFail);
		}
	}
	else
	{
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("Incorrectly formatted message: %s", szData);
		);
	}

	return (eReturn);
}


/*!
 * \brief Changes a persistent data item
 */
EstiResult CstiVRCLServer::SettingChange (
	char * szParms,  // string containing attributes
	char * szData,   // string containing new value (and end tag)
	uint32_t un32DataSize) // Size of received data
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::SettingChange);

	EstiResult eResult = estiERROR;

	if (nullptr != szParms && nullptr != szData)
	{
		const int nNAME_LEN = 48;
		const int nTYPE_LEN = 24;
		char szType[nTYPE_LEN];
		char szName[nNAME_LEN];

		//
		// Initialize the strings to empty.
		//
		szType[0] = '\0';
		szName[0] = '\0';
		
		// Read the attributes
		// The string pointed to by szParms should look like 'Type="xxx" Name="yyy">zzz</SettingSet'
		// where "xxx" is the type of value (Number, String, Enum),
		//       "yyy" is the name of the setting in the PropertyManager and
		//       "zzz" is the value to be stored.
		char* szEqual = strstr (szParms, "=");  // Find the equal sign prior to the Type
		if (nullptr != szEqual)
		{
			int i = 0;  // Start just past the equal sign and the opening quote mark

			// Get the type
			while (szEqual[i + 2] != '"' && i < nTYPE_LEN)
			{
				szType[i] = szEqual[i + 2];
				i++;
			}

			// Terminate the string
			szType[i] = 0;

			szEqual = strstr (szEqual + 1, "=");  // Find the second equal sign (prior to the Name)
			if (nullptr != szEqual)
			{
				int i = 0;  // Start just past the equal sign and the opening quote mark

				// Get the name
				while (szEqual[i + 2] != '"' && i < nNAME_LEN)
				{
					szName[i] = szEqual[i + 2];
					i++;
				}

				// Terminate the string
				szName[i] = 0;
			}
		}

		// Do we have a property name and type?
		if (0 < strlen (szName) && 0 < strlen (szType))
		{
			eResult = estiOK;
			
			SstiPropertyInfo Property;
			std::string value (szData, un32DataSize);

			// un-escape any special characters
			auto unescaped = strXunescape (value.c_str ());
			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				vpe::trace ("CstiVRCLServer::SettingChange ", szName, "(", szType, ") changing to ", unescaped, "\n");
			);

			Property.Name = szName;
			Property.Type = szType;
			Property.Value = unescaped;

			m_PropertyList.push_back (Property);
		}
		else
		{
			stiDEBUG_TOOL (g_stiVRCLServerDebug,
				stiTrace ("CstiVRCLServer::SettingChange "
					"Unable to decode the parameters <%s>.  Nothing updated.\n", szParms);
			);
		}
	}
	else
	{
		stiDEBUG_TOOL (g_stiVRCLServerDebug,
			stiTrace ("CstiVRCLServer::SettingChange "
				"Parameters passed in are NULL.  Nothing updated.\n");
		);
	}

	return (eResult);
}


/*!
 * \brief Sends the current state to the client
 */
EstiResult CstiVRCLServer::StateSend ()
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::StateSend);

	EstiResult eResult = estiOK;

	if (m_pConferenceManager->eventLoopIsRunning ())
	{
		// Since we are in the ready state, we need to determine if there
		// are any active calls.  If so, what state are they in?
		// If we have more than one call in the system, return "AMBIGUOUS",
		// otherwise, return the state of the active call.
		if (m_externalCall != nullptr)
		{
			switch (m_pConferenceManager->CallObjectsCountGet ())
			{
				default:
				case 2:
					eResult = ClientResponseSend (g_szStatusMultipleCalls);
					break;

				case 1:
				{
					auto call = callObjectHeadGet ();
					if (call)
					{
						StateSend (call);
					}
					else
					{
						eResult = ClientResponseSend (g_szStatusIdle);
					}
				}
				break;

				case 0:
					// Write the response to the client
					eResult = ClientResponseSend (g_szStatusIdle);
					break;
			} // end switch (# of call objects)
		}
	}
	else
	{
		// Write the response to the client
		eResult = ClientResponseSend (g_szStatusShutdown);

	}

	return (eResult);
}


/*!
 * \brief Sends the current state to the client
 */
EstiResult CstiVRCLServer::StateSend (
    VRCLCallSP call)
{
	EstiResult eResult = estiERROR;

	//
	// Are we connected to a client?
	//
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		if (call)
		{
			EsmiCallState eCallState = call->StateGet ();

			// We have the call object, what state is it in?
			switch (eCallState)
			{
				case esmiCS_HOLD_LCL:
				case esmiCS_HOLD_RMT:
				case esmiCS_HOLD_BOTH:
				{
					// We are on hold (or transitioning to/from hold).
					// Create a message to the client indicating with
					// whom we are connected.

					// Send a full message to the Client with our status
					const char *pszMessage = nullptr;

					// Add the hold type
					switch (eCallState)
					{
						case esmiCS_HOLD_LCL:
							pszMessage = g_szStatusOnHoldLocal;
							break;

						case esmiCS_HOLD_RMT:
							pszMessage = g_szStatusOnHoldRemote;
							break;

						case esmiCS_HOLD_BOTH:
							pszMessage = g_szStatusOnHoldBoth;
							break;

						default:
							break;
					}

					// Send the message to the client.
					eResult = ClientResponseSend (pszMessage);

					break;
				}

				case esmiCS_INIT_TRANSFER:
				case esmiCS_TRANSFERRING:
				case esmiCS_CONNECTED:
					eResult = ConnectedStateSend (call);
					break;

				case esmiCS_CONNECTING:
				{
					std::string message {g_szStatusConnecting};

					switch (call->DirectionGet ())
					{
						case estiUNKNOWN:

							message += g_szUnknown;

							break;

						case estiINCOMING:

							message += g_szIncomingCall;

							break;

						case estiOUTGOING:

							message += g_szOutgoingCall;
							break;
					}

					message += g_szStatusConnectingEnd;

					eResult = ClientResponseSend (message.c_str ());

					break;
				}

				case esmiCS_DISCONNECTING:
					eResult = ClientResponseSend (g_szStatusDisconnecting);
					break;

				case esmiCS_DISCONNECTED:
					if (call->LeavingMessageGet())
					{
						eResult = ClientResponseSend (g_szStatusLeavingMessage);
					}
					else
					{
						eResult = ClientResponseSend (g_szStatusIdle);
					}

					break;

				case esmiCS_IDLE:
				case esmiCS_UNKNOWN:
				case esmiCS_CRITICAL_ERROR:
					eResult = ClientResponseSend (g_szStatusError);
					break;
			} // end switch
		}
	}
	else
	{
		// If a client is not connected go ahead and return success.
		// This matches the behavior of other messages when a client is not attached.
		eResult = estiOK;
	}

	return eResult;
}


/*!
 * \brief Sends the current in call stats to the client
 */
stiHResult CstiVRCLServer::InCallStatsSend (
    VRCLCallSP call)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//
	// Are we connected to a client?
	//
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		if (call)
		{
			std::stringstream InCallStatus;

			SstiCallStatistics stCallStatistics;
			call->StatisticsGet(&stCallStatistics);
			EstiProtocol eProtocol = call->ProtocolGet ();

			InCallStatus << g_szInCallStats;
			InCallStatus << g_szCurrentCallStats;
			InCallStatus << g_szDuration << stCallStatistics.dCallDuration << g_szDurationEnd;
			InCallStatus << g_szProtocol << (estiSIP == eProtocol ? "SIP" : "Unknown") << g_szProtocolEnd;
			InCallStatus << g_szVideoCodecsSent <<
				(estiVIDEO_NONE == stCallStatistics.Record.eVideoCodec ? "None" :
				 estiVIDEO_H263 == stCallStatistics.Record.eVideoCodec ? "H263" :
				 estiVIDEO_H264 == stCallStatistics.Record.eVideoCodec ? "H264" : "Unknown") << g_szVideoCodecsSentEnd;
			InCallStatus << g_szVideoCodecsRecv <<
				(estiVIDEO_NONE == stCallStatistics.Playback.eVideoCodec ? "None" :
				 estiVIDEO_H263 == stCallStatistics.Playback.eVideoCodec ? "H263" :
				 estiVIDEO_H264 == stCallStatistics.Playback.eVideoCodec ? "H264" : "Unknown") << g_szVideoCodecsRecvEnd;

			InCallStatus << g_szVideoFrameSizeSent << stCallStatistics.Record.nVideoWidth << " x " << stCallStatistics.Record.nVideoHeight << g_szVideoFrameSizeSentEnd;
			InCallStatus << g_szVideoFrameSizeRecv << stCallStatistics.Playback.nVideoWidth << " x " << stCallStatistics.Playback.nVideoHeight << g_szVideoFrameSizeRecvEnd;

			InCallStatus << g_szTargetFPS << stCallStatistics.Record.nTargetFrameRate << g_szTargetFPSEnd;
			InCallStatus << g_szActualFPSSent << stCallStatistics.Record.nActualFrameRate << g_szActualFPSSentEnd;
			InCallStatus << g_szActualFPSRecv << stCallStatistics.Playback.nActualFrameRate << g_szActualFPSRecvEnd;
			InCallStatus << g_szTargetVideoKbps << stCallStatistics.Record.nTargetVideoDataRate << g_szTargetVideoKbpsEnd;
			InCallStatus << g_szActualVideoKbpsSent << stCallStatistics.Record.nActualVideoDataRate << g_szActualVideoKbpsSentEnd;
			InCallStatus << g_szActualVideoKbpsRecv << stCallStatistics.Playback.nActualVideoDataRate << g_szActualVideoKbpsRecvEnd;
			InCallStatus << g_szReceivedPackeLossPercent << stCallStatistics.fReceivedPacketsLostPercent << g_szReceivedPackeLossPercentEnd;
			InCallStatus << g_szCurrentCallStatsEnd;

			InCallStatus << g_szReceivedVideoStats;
			InCallStatus << g_szTotalPacketsReceived << stCallStatistics.Playback.totalPacketsReceived << g_szTotalPacketsReceivedEnd;
			InCallStatus << g_szTotalFramesAssembledAndSentToDecoder << stCallStatistics.un32FrameCount << g_szTotalFramesAssembledAndSentToDecoderEnd;
			InCallStatus << g_szLostPackets << stCallStatistics.Playback.totalPacketsLost << g_szLostPacketsEnd;
			InCallStatus << g_szVideoPlaybackFrameGetErrors << stCallStatistics.un32VideoPlaybackFrameGetErrors << g_szVideoPlaybackFrameGetErrorsEnd;
			
			InCallStatus << g_szKeyframesRequested << stCallStatistics.un32KeyframesRequestedVP << g_szKeyframesRequestedEnd;
			InCallStatus << g_szKeyframesReceived << stCallStatistics.un32KeyframesReceived << g_szKeyframesReceivedEnd;
			InCallStatus << g_szKeyframeWaitTimeMax << stCallStatistics.un32KeyframeMaxWaitVP << g_szKeyframeWaitTimeMaxEnd;
			InCallStatus << g_szKeyframeWaitTimeMin << stCallStatistics.un32KeyframeMinWaitVP << g_szKeyframeWaitTimeMinEnd;
			InCallStatus << g_szKeyframeWaitTimeAvg << stCallStatistics.un32KeyframeAvgWaitVP << g_szKeyframeWaitTimeAvgEnd;
			InCallStatus << g_szKeyframeWaitTimeTotal << stCallStatistics.fKeyframeTotalWaitVP << g_szKeyframeWaitTimeTotalEnd;
			InCallStatus << g_szErrorConcealmentEvents << stCallStatistics.un32ErrorConcealmentEvents << g_szErrorConcealmentEventsEnd;
			InCallStatus << g_szIFramesDiscardedAsIncomplete << stCallStatistics.un32IFramesDiscardedIncomplete << g_szIFramesDiscardedAsIncompleteEnd;
			InCallStatus << g_szPFramesDiscardedAsIncomplete << stCallStatistics.un32PFramesDiscardedIncomplete << g_szPFramesDiscardedAsIncompleteEnd;
			InCallStatus << g_szGapsInFrameNumbers << stCallStatistics.un32GapsInFrameNumber << g_szGapsInFrameNumbersEnd;
			InCallStatus << g_szReceivedVideoStatsEnd;

			InCallStatus << g_szSentVideoStats;
			InCallStatus << g_szKeyframesRequested << stCallStatistics.un32KeyframesRequestedVR << g_szKeyframesRequestedEnd;
			InCallStatus << g_szKeyframesSent << stCallStatistics.un32KeyframesSent << g_szKeyframesSentEnd;
			InCallStatus << g_szKeyframeWaitTimeMax << stCallStatistics.un32KeyframeMaxWaitVR << g_szKeyframeWaitTimeMaxEnd;
			InCallStatus << g_szKeyframeWaitTimeMin << stCallStatistics.un32KeyframeMinWaitVR << g_szKeyframeWaitTimeMinEnd;
			InCallStatus << g_szKeyframeWaitTimeAvg << stCallStatistics.un32KeyframeAvgWaitVR << g_szKeyframeWaitTimeAvgEnd;
			InCallStatus << g_szFramesLostFromRCU << stCallStatistics.un32FramesLostFromRcu << g_szFramesLostFromRCUEnd;
			InCallStatus << g_szPartialFramesSent << stCallStatistics.countOfPartialFramesSent << g_szPartialFramesSentEnd;
			InCallStatus << g_szSentVideoStatsEnd;
			InCallStatus << g_szInCallStatsEnd;
			EstiResult eResult = ClientResponseSend (InCallStatus.str().c_str ());
			stiTESTCOND(eResult==estiOK, stiRESULT_ERROR);

		} // end if
		else
		{
			char szMessage [256];
			sprintf (szMessage, "%s%s%s", g_szInCallStats, g_szNoActiveCall, g_szInCallStatsEnd);
			EstiResult eResult = ClientResponseSend (szMessage);
			stiTESTCOND(eResult==estiOK, stiRESULT_ERROR);
		}
	}
STI_BAIL:
	return hResult;
}


/*!
 * \brief Converts a string to an XML-escaped string
 *
 * \return A pointer to the newly escaped string or NULL in the case the
 *         buffer wasn't long enough to receive it.  In the error case,
 *         szLen will contain the length of the buffer required
 *         (minus the NULL terminator)
 */
char* CstiVRCLServer::strXescape (
	const char* szData,
	char* szBuf,
	int* nLen)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::strXescape);
	int i = 0;
	int j = 0;
	char* szRtn = nullptr;
	int nReqLen = strXlen (szData);

	// First check to see that the buffer provided is large enough.
	if (*nLen > nReqLen)
	{
		// Initialize the passed in buffer to fill
		memset (szBuf, 0, *nLen);

		while (szData[i])
		{
			// For certain characters, XML has to escape them for the parser to
			// work correctly.  These characters are "<>&'" (not including the
			// quotes).  We aren't concerned with the "'" character since it is
			// only an issue if used within attributes and we don't use attributes
			// in VRCL.
			// If the current character being copied is one of these, replace it
			// with the escaped character sequence.
			if ('<' == szData[i])
			{
				strcat (szBuf, "&lt;");
				j += nLT_LEN;
			}
			else if ('>' == szData[i])
			{
				strcat (szBuf, "&gt;");
				j += nGT_LEN;
			}
			else if ('&' == szData[i])
			{
				strcat (szBuf, "&amp;");
				j += nAMP_LEN;
			}
			else
			{
				szBuf[j++] = szData[i];
			}

			i++;
		}
		szRtn = szBuf;
	}
	else
	{
		*nLen = nReqLen;
	}

	return (szRtn);
}


/*!
 * \brief Takes an ordinary stirng and determines what the length of it will be
 *        when any special characters "<>&" are escaped.
 */
int CstiVRCLServer::strXlen (const char* szBuf)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::strXlen);

	int nBufLen = 0;
	int i = 0;
	while (szBuf[i])
	{
		// For certain characters, XML has to escape them for the parser to
		// work correctly.  These characters are "<>&'" (not including the
		// quotes).  We aren't concerned with the "'" character since it is
		// only an issue if used within attributes and we don't use attributes
		// in VRCL.
		// If the current character being counted is one of these, add the
		// necessary spaces to accommodate the escaped character.
		if ('<' == szBuf[i])
		{
			nBufLen += nLT_LEN;
		}
		else if ('>' == szBuf[i])
		{
			nBufLen += nGT_LEN;
		}
		else if ('&' == szBuf[i])
		{
			nBufLen += nAMP_LEN;
		}
		else
		{
			nBufLen++;
		}

		i++;
	}

	return (nBufLen);
}


/*!\brief Addds an XML node with the provided tag and integer value
 *
 * @param pMessage The string stream to which the node is added
 * @param pszTagName The name of the tag
 * @param nValue The value stored between the begin tag and end tag
 */
static void NodeAdd (
	std::stringstream *pMessage,
	const char *pszTagName,
	int nValue)
{
	*pMessage << "<" << pszTagName << ">" << nValue << "</" <<pszTagName << ">";
}


void CstiVRCLServer::UserInfoAdd (
    VRCLCallSP call,
	std::stringstream *pMessage)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::UserInfoAdd);

	char szTmp[nMAX_MESSAGE_BUFFER];
	int nTmp = nMAX_MESSAGE_BUFFER;

	if (call)
	{
		std::string RemoteName;
		std::string IPAddress;
		std::string ProductName;
		std::string ProductVersion;

		call->RemoteNameGet (&RemoteName);

		const char* szMacAddress = call->RemoteMacAddressGet ();
		call->RemoteProductVersionGet (&ProductVersion);

		call->RemoteProductNameGet (&ProductName);
		
		IPAddress = call->TrsUserIPGet ();
		
		// If we did not get an IP address from Call-Info see if we were able
		// to get one from the VIA or SINFO.
		if (IPAddress.empty ())
		{
			IPAddress = call->RemoteIpAddressGet ();
		}

		auto poCallInfo = call->RemoteCallInfoGet ();
		const SstiUserPhoneNumbers *psUserPhoneNumbers = poCallInfo->UserPhoneNumbersGet ();
		const char* szVcoCallback = call->RemoteVcoCallbackGet ();

		// Add the user's data
		*pMessage << g_szName << strXescape (RemoteName.c_str (), szTmp, &nTmp) << g_szNameEnd;
		*pMessage << g_szIPAddress << IPAddress << g_szIPAddressEnd;
		*pMessage << g_szPhoneNumber << psUserPhoneNumbers->szPreferredPhoneNumber << g_szPhoneNumberEnd;

		if (call->IsTransferableGet () ||
		   estiINCOMING == call->DirectionGet ())
		{
			*pMessage << g_szCallIsTransferable;
		}
		else
		{
			*pMessage << g_szCallIsNotTransferable;
		}

		*pMessage << g_szPhoneNumberTollFree << psUserPhoneNumbers->szTollFreePhoneNumber << g_szPhoneNumberTollFreeEnd;
		*pMessage << g_szPhoneNumberLocal << psUserPhoneNumbers->szLocalPhoneNumber << g_szPhoneNumberLocalEnd;
		*pMessage << g_szPhoneNumberSorenson << psUserPhoneNumbers->szSorensonPhoneNumber << g_szPhoneNumberSorensonEnd;

		*pMessage << g_szPhoneNumberRingGroupLocal << psUserPhoneNumbers->szRingGroupLocalPhoneNumber << g_szPhoneNumberRingGroupLocalEnd;
		*pMessage << g_szPhoneNumberRingGroupTollFree << psUserPhoneNumbers->szRingGroupTollFreePhoneNumber << g_szPhoneNumberRingGroupTollFreeEnd;

		std::string userId;
		poCallInfo->UserIDGet (&userId);

		*pMessage << g_szUserId << userId << g_szUserIdEnd;

		std::string groupUserId;
		poCallInfo->GroupUserIDGet (&groupUserId);

		*pMessage << g_szGroupUserId << groupUserId << g_szGroupUserIdEnd;

		*pMessage << g_szProtocol;

		switch (call->ProtocolGet ())
		{
			case estiPROT_UNKNOWN:
				*pMessage << g_szPROTOCOL_UNKOWN;
				break;
			case estiSIP:
				*pMessage << g_szPROTOCOL_SIP;
				break;
			case estiSIPS:
				*pMessage << g_szPROTOCOL_SIPS;
				break;
			// default: Keep this commented out, so type checking will warn you of mistakes!
		}
		*pMessage << g_szProtocolEnd;

		*pMessage << g_szVerifyAddress << (call->VerifyAddressGet () ? 1 : 0) << g_szVerifyAddressEnd;

		NodeAdd (pMessage, g_szVcoPref, call->RemoteVcoTypeGet ());
		NodeAdd (pMessage, g_szVcoActive, call->RemoteIsVcoActiveGet() ? 1 : 0);

		if (call->TextSupportedGet ())
		{
			*pMessage << g_szRealTimeTextSupported;
		}

		// Is there a VCO callback number?
		if (strlen (szVcoCallback) > 0)
		{
			// Yes! Include it.
			*pMessage << g_szVCO << szVcoCallback << g_szVCOEnd;
		}

		*pMessage << g_szMACAddress << szMacAddress << g_szMACAddressEnd;
		*pMessage << g_szProductName << ProductName << g_szProductNameEnd;
		*pMessage << g_szProductVersion << ProductVersion << g_szProductVersionEnd;
	}
}

/* \brief Handle the XML command to send a text backspace
 *
 * Expected format:
 * 	<TextBackspaceSend />
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::TextBackspaceSend ()
{
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);

	auto call = callObjectGet (esmiCS_CONNECTED);
	if (call && call->TextSupportedGet ())
	{
		auto pun16TextCopy = new uint16_t[2];

		// Copy in the backspace char (including the terminator (0x0000)).
		pun16TextCopy[0] = (uint16_t)0x08;
		pun16TextCopy[1] = (uint16_t)0;

		hResult = call->TextSend (pun16TextCopy, estiSTS_KEYBOARD);
		stiTESTRESULT ();

		// Call CCI with a copy of the text that was sent
		textMessageSentSignal.Emit (pun16TextCopy);
	}

STI_BAIL:
	return hResult;
}


/* \brief Handle the XML command to send a text message
 * \param pRootNode - the node that contains the TextMessageSend command
 * 
 * Expected format:
 * 	<TextMessageSend>text</TextMessageSend>
 * 
 * \return stiHResult
 */
stiHResult CstiVRCLServer::TextMessageSend (IXML_Node *pRootNode)
{
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);
	uint16_t *pwszTextMessage = nullptr;
	
	auto call = callObjectGet (esmiCS_CONNECTED);
	if (call && call->TextSupportedGet ())
	{
		hResult = GetTextNodeValueWide (pRootNode, &pwszTextMessage);
		stiTESTRESULT ();

		hResult = call->TextSend (pwszTextMessage, estiSTS_KEYBOARD);
		stiTESTRESULT ();

		// Prepare to send received text up to the UI of the VP
		// First, Determine the length of the string and allocate new string
		int nLength;
		for (nLength = 0; pwszTextMessage[nLength]; nLength++)
		{
		}
		
		auto pun16TextCopy = new uint16_t[nLength + 1];

		// Copy the data (including the terminator (0x0000)).
		memcpy (pun16TextCopy, pwszTextMessage, sizeof (uint16_t) * (nLength + 1));

		// Call CCI with a copy of the text that was sent
		textMessageSentSignal.Emit (pun16TextCopy);
	}
	
STI_BAIL:

	if (pwszTextMessage)
	{
		delete [] pwszTextMessage;
		pwszTextMessage = nullptr;
	}
	
	return hResult;
}


/* \brief Handle the XML command to clear text messages
 * \param pRootNode - the node that contains the TextMessageClear command
 * 
 * Expected format:
 * 	<TextMessageClear />
 * 
 * \return stiHResult
 */
stiHResult CstiVRCLServer::TextMessageClear (IXML_Node *pRootNode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	//TODO Implement External Version of Text Share Remove remote and add hresult to call
	auto call = m_pConferenceManager->CallObjectGet (esmiCS_CONNECTED);
	if (call && call->TextSupportedGet ())
	{
		call->TextShareRemoveRemote ();
	}
	
	return hResult;
}


/**
 * \brief Handle the XML command to set the terpnet mode
 * \param pRootNode - the node that contains the TerpNetModeSet command
 *
 * Expected format:
 * 	<TerpNetModeSet>text</TerpNetModeSet>
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::TerpNetModeSet (IXML_Node *pRootNode)
{
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);
	std::string text;

	hResult = GetTextNodeValue (pRootNode, text);
	stiTESTRESULT ();

	// Call CCI with a copy of the text that was sent
	terpNetModeSetSignal.Emit (text);

STI_BAIL:

	return hResult;
}


/**
 * \brief Handle the XML command HearingStatus
 * \param pRootNode - the node that contains the HearingStatus command
 *
 * Expected format:
 * 	<HearingStatus>status_text</HearingStatus>
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::HearingStatus (IXML_Node *pRootNode)
{
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);
	std::string text;

	hResult = GetTextNodeValue (pRootNode, text);
	stiTESTRESULT ();

	// Prepare to send received status to deaf customer VP
	EstiHearingCallStatus eHearingStatus;
	
	if (text == "Connected")
	{
		eHearingStatus = estiHearingCallConnected;
	}
	else if (text == "Connecting")
	{
		eHearingStatus = estiHearingCallConnecting;
	}
	else // if strcmp(pszText, "Disconnected") == 0)
	{
		eHearingStatus = estiHearingCallDisconnected;
	}

	// Call CCI with the status that was sent
	hearingStatusChangedSignal.Emit (eHearingStatus);

STI_BAIL:

	return hResult;
}


/**
 * \brief Handle the XML command HearingVideoCapability
 * \param pRootNode - the node that contains the 
 *  	  HearingVideoCapabilty command
 *
 * Expected format:
 *  <HearingVideoCapability>
 *  	<HearingNumber>phone#</HearingNumber>
 *  </HearingVideoCapability>
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::hearingVideoCapability (IXML_Node *rootNode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string nodeName;
	std::string nodeValue;
	IXML_Node *node {nullptr};
	std::string hearingNumber;

	hResult = GetChildNodeNameAndValue (rootNode, nodeName, nodeValue, &node);
	stiTESTRESULT ();

	// Call CCI with the hearing number that was sent
	hearingVideoCapabilitySignal.Emit (nodeValue);

STI_BAIL:

	return hResult;
}


/**
 * \brief Handle the XML command RelayNewCallReady
 * \param pRootNode - the node that contains the RelayNewCallReady command
 *
 * Expected format:
 * 	<RelayNewCallReady>status_text</RelayNewCallReady>
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::RelayNewCallReady (IXML_Node *pRootNode)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	std::string text;

	hResult = GetTextNodeValue (pRootNode, text);
	stiTESTRESULT ();

	// Prepare to send received ready status to deaf customer VP
	{
		bool bRelayNewCallReady = text == "True";

		// Call CCI with the status that was sent
		relayNewCallReadySignal.Emit (bRelayNewCallReady);
	}

STI_BAIL:

	return hResult;
}


/* \brief Handle the XML command to dial a phone number
 * \param pRootNode - the node that contains the PhoneNumberDial command
 * 
 * \return stiHResult
 */
stiHResult CstiVRCLServer::PhoneNumberDial (IXML_Node *pRootNode)
{
	stiDEBUG_TOOL (g_stiVRCLServerDebug,
		stiTrace ("CstiVRCLServer::PhoneNumberDial Received %s\n", g_szPhoneNumberDialMessage);
	);
	
	// If this is from a 3.1 (or later) version of the API, it could look like:
	/* 
	 * <PhoneNumberDial>
	 * 	<CallId>AAA</CallId>
	 * 	<TerpId>BBB</TerpId>
	 * 	<HearingNumber>CCC-CCC-CCCC</HearingNumber>
	 * 	<HearingName>DDD</HearingName>
	 * 	<DestNumber>XXX-XXX-XXXX</DestNumber>
	 * 	<AlwaysAllow>false/true</AlwaysAllow>
	 * </PhoneNumberDial>
	 */
	
	// OR... for 3.1 (or later), it could look like:
	/* 
	 * <PhoneNumberDial>
	 * 	<DestNumber>XXX-XXX-XXXX</DestNumber>
	 * 	<AlwaysAllow>false/true</AlwaysAllow>
	 * </PhoneNumberDial>
	 */
	
	// If this is from pre-3.1, it will look like:
	/* 
	 * <PhoneNumberDial>XXX-XXX-XXXX</PhoneNumberDial>
	 */
	
	std::string nodeName;
	std::string nodeValue;
	IXML_Node *pNode = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;
	SstiCallDialInfo stCallDialInfo;

	stCallDialInfo.eDialMethod = estiDM_BY_DS_PHONE_NUMBER;

	// See if it is the pre-3.1 format by simply looking for the text node value
	std::string phone{ nodeTextValueGet(pRootNode) };

	// If PhoneNumberDial didn't have text value, then it isn't the pre-3.1 format.
	if (phone.empty())
	{
		pNode = ixmlNode_getFirstChild(pRootNode);
		stiTESTCONDMSG(pNode, stiRESULT_ERROR, "Empty <PhoneNumberDial>");

		while (pNode)
		{
			phoneNumberDialNodeProcess(pNode, stCallDialInfo);
			pNode = ixmlNode_getNextSibling(pNode);
		}
	}
	else  // Pre 3.1  API
	{
		stiRemoteLogEventSend("EventType=VRCL_API_VERSION Reason=PhoneNumberDial called with pre 3.1 API");
		stCallDialInfo.DestNumber = phone;
		stCallDialInfo.HearingNumber = m_szHearingPhoneNumber;
	}

	callDialSignal.Emit (stCallDialInfo);
	
	// Blank out the hearing number now since it will be stored in the
	// call object at this point.
	m_szHearingPhoneNumber[0] = '\0';
	
STI_BAIL:

	return hResult;
}


/* \brief Handle the XML command to do a firmware update check
 * \param pRootNode - the node that contains the UpdateCheck command
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::UpdateCheck (IXML_Node *pRootNode)
{
	stiDEBUG_TOOL (g_stiVRCLServerDebug,
						stiTrace ("CstiVRCLServer::UpdateCheck Received %s\n", g_szUpdateCheck);
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	// Check if we are in a call
	if (m_pConferenceManager->eventLoopIsRunning ()
	&& 0 == m_pConferenceManager->CallObjectsCountGet ())
	{
		//Pass the update message to CCI
		updateCheckNeededSignal.Emit ();
	}
	else
	{
		//We are in a call so send the 'InCall' responce to the client
		UpdateCheckResult (CstiVRCLServer::estiUPDATE_IN_CALL);
	}

	return hResult;
}


/*!
 * \brief Called by VRCLNotify or UpdateCheck and schedules the results of the update
 *        to be send to the VRCL client
 */
void CstiVRCLServer::UpdateCheckResult(
    int32_t message)
{
	PostEvent ([this,message]{ eventUpdateCheckResult(message); });
}


/*!
 * \brief Notifies the VRCL Client whether the update succeded or not
 */
void CstiVRCLServer::eventUpdateCheckResult (
    int32_t status)
{
#if 0
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventUpdateCheckResult);
	
	// Are we connected to a client?
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		char szElementVal[60];

		switch (status)
		{
		    // TODO:  Not a good idea to switch over multiple enumerations like this...
		    //        This entry conflicts with estiMSG_OUTGOING_CALL
			case CstiVRCLServer::estiUPDATE_IN_CALL:
				snprintf(szElementVal, sizeof (szElementVal), "<%s>%s</%s>", g_szUpdateCheckResult, g_szInCall, g_szUpdateCheckResult);
				break;

			case estiMSG_UPDATE_UP_TO_DATE:
				snprintf(szElementVal, sizeof (szElementVal), "<%s>%s</%s>", g_szUpdateCheckResult, g_szAppVersionOK, g_szUpdateCheckResult);
				break;

			case estiMSG_UPDATE_SUCCESSFUL:
				snprintf(szElementVal, sizeof (szElementVal), "<%s>%s</%s>", g_szUpdateCheckResult, g_szSuccess, g_szUpdateCheckResult);
				break;

			case estiMSG_UPDATE_ERROR:
				snprintf(szElementVal, sizeof (szElementVal), "<%s>%s</%s>", g_szUpdateCheckResult, g_szFail, g_szUpdateCheckResult);
				break;
		}
	
		ClientResponseSend ((const char *)&szElementVal);
	}
#endif
}


/* \brief Handle the XML command to do set the list of supported codecs
 * \param pRootNode - the node that contains the AllowedAudioCodecs command
 *
 * Expected Format:
 * 	<AllowedAudioCodecs><Codec />...</AllowedAudioCodecs>
 *  Where each Codec is the same as the name in SIP SDP.
 * 
 * \return stiHResult
 */
stiHResult CstiVRCLServer::AllowedAudioCodecs (IXML_Node *pRootNode)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	SstiConferenceParams stConferenceParams;
	hResult = m_pConferenceManager->ConferenceParamsGet (&stConferenceParams);
	stiTESTRESULT ();
	
	stConferenceParams.AllowedAudioCodecs.clear ();
	for (IXML_Node *pChildNode = ixmlNode_getFirstChild (pRootNode); pChildNode; pChildNode = ixmlNode_getNextSibling (pChildNode))
	{
		const char *pszName = ixmlNode_getNodeName (pChildNode);
		stConferenceParams.AllowedAudioCodecs.push_back (pszName);
	}

	hResult = m_pConferenceManager->ConferenceParamsSet (&stConferenceParams);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		ClientResponseSend (g_szAllowedAudioCodecsFail);
	}
	else
	{
		ClientResponseSend (g_szAllowedAudioCodecsSuccess);
	}
	
	return hResult;
}


/* \brief Handle the XML command to do a debug trace flag set
 * \param pRootNode - the node that contains the CallIdSet command
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::CallIdSet (IXML_Node *pRootNode)
{
	stiDEBUG_TOOL (g_stiVRCLServerDebug,
		stiTrace ("CstiVRCLServer::CallIdSet Received %s\n", g_szCallIdSet);
	);

	stiHResult hResult = stiRESULT_SUCCESS;
	std::string value;

	//Get the Call ID value
	hResult = GetTextNodeValue (pRootNode, value);
	stiTESTRESULT ();

	// Call CCI with the VRS Call Id value
	vrsCallIdSetSignal.Emit (value);

	CallIdSetResult (CstiVRCLServer::estiVRS_CALL_ID_SET_SUCCESSFUL);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		CallIdSetResult (CstiVRCLServer::estiVRS_CALL_ID_SET_FAIL);
	}

	return hResult;
}


/*!
 * \brief Schedules the notification to VRCL Client of CallIdSet results
 */
void CstiVRCLServer::CallIdSetResult(
	int32_t message)
{
	PostEvent ([this,message]{
		eventCallIdSetResult((ECallIdSetResult)message);
	});
}


/*!
 * \brief Notifies the VRCL Client whether the debug set succeeded or not
 */
void CstiVRCLServer::eventCallIdSetResult (
	ECallIdSetResult result)
{
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventCallIdSetResult);

	// Are we connected to a client
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		char szElementVal[60];

		switch (result)
		{
			case estiVRS_CALL_ID_SET_SUCCESSFUL:
				snprintf(szElementVal, sizeof (szElementVal), "<%s>%s</%s>", g_szCallIdSetResult, g_szSuccess, g_szCallIdSetResult);
				break;

			case estiVRS_CALL_ID_SET_FAIL:
				snprintf(szElementVal, sizeof (szElementVal), "<%s>%s</%s>", g_szCallIdSetResult, g_szFail, g_szCallIdSetResult);
				break;
		}

		ClientResponseSend ((const char *)&szElementVal);
	}
}


/*!
 * \brief CstiVRCLServer::LoggedInPhoneNumberSet
 */
void CstiVRCLServer::LoggedInPhoneNumberSet (const char *pszLoggedInPhoneNumber)
{
	stiLOG_ENTRY_NAME(CstiVRCLServer::LoggedInPhoneNumberSet);

	m_strLoggedInPhoneNumber = pszLoggedInPhoneNumber;

}


/*!
 * \brief  Verify the deaf VP reecived the hearing status changed notification (async)
 */
void CstiVRCLServer::HearingStatusResponse()
{
	PostEvent ([this]{ eventHearingStatusResponse(); });
}


/*!
 * \brief Event handler to notify the VRCL Client that the hearing status change
 *        message was received by the deaf VP
 */
void CstiVRCLServer::eventHearingStatusResponse ()
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client.
		char szMessage[nMAX_MESSAGE_BUFFER];
		sprintf (szMessage, "<%s>%s</%s>",
				g_szHearingStatusResponse, g_szSuccess, g_szHearingStatusResponse);

		stiTESTCOND (estiOK == ClientResponseSend(szMessage), stiRESULT_ERROR);
	}

STI_BAIL:
	return;
}


/*!
 * \brief Verifies the deaf VP erecived the RelayNewCallReady message (async)
 */
void CstiVRCLServer::RelayNewCallReadyResponse()
{
	PostEvent ([this]{ eventRelayNewCallReadyResponse(); });
}


/*!
 * \brief Event handler to notify the VRCL Client tha the RelayNewCallReady message
 *        was reecived by the deaf VP
 */
void CstiVRCLServer::eventRelayNewCallReadyResponse ()
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);

	// If we are connected to a client...
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		// Send a message to the Client.
		char szMessage[nMAX_MESSAGE_BUFFER];
		sprintf (szMessage, "<%s>%s</%s>",
				g_szRelayNewCallReadyResponse, g_szSuccess, g_szRelayNewCallReadyResponse);

		stiTESTCOND (estiOK == ClientResponseSend(szMessage), stiRESULT_ERROR);
	}

STI_BAIL:
	return;
}


/*!
 * \brief Called by VP engine to send a phone number to do VRS call spawn (async)
 */
void CstiVRCLServer::HearingCallSpawn(
    const SstiSharedContact &contact)
{
	PostEvent ([this, contact]{ eventHearingCallSpawn(contact); });
}


/*!
 * \brief Event handler that notifies the VRCL Client of a shared phone number to
 *        spawn a new VRC call
 */
void CstiVRCLServer::eventHearingCallSpawn (
	const SstiSharedContact &contact)
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventHearingCallSpawn);

	// Spawn call (via share contact) isn't available until 4.0
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		if (!(contact.DialString).empty())
		{
			// Send a message to the Client.
			char szMessage[nMAX_MESSAGE_BUFFER];
			sprintf (szMessage,
			        "<%s>%s%s%s</%s>",
			        g_szHearingCallSpawn,
			        g_szPhoneNumber,
			        (contact.DialString).c_str(),
			        g_szPhoneNumberEnd,
			        g_szHearingCallSpawn);

			stiTESTCOND (estiOK == ClientResponseSend(szMessage), stiRESULT_ERROR);
		}
	}

STI_BAIL:
	return;
}


/*!\brief Posts an event on the VRCLServer queue that geolocation has changed for the remote endpoint
 *
 * \param std::string, geolocation
 */
void CstiVRCLServer::remoteGeolocationChanged (
	const std::string& geolocation)
{
	PostEvent ([this, geolocation]{eventRemoteGeolocationChanged (geolocation); });
}


/*!\brief Handles geolocation changed event of remote endpoint and sends geolocation string to client
 *
 * \param std::string, geolocation
 */
void CstiVRCLServer::eventRemoteGeolocationChanged (
	const std::string& geolocation)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG (hResult); // Prevent compiler warning
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventRemoteGeolocationChanged);
	
	//
	// Are we connected to a client
	//
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		if (!geolocation.empty ())
		{
			std::ostringstream message;
			message << "<" << g_szGeolocation << ">";
			message << geolocation;
			message << "</" << g_szGeolocation << ">";
			
			auto result = ClientResponseSend (message.str ().c_str ());
			
			stiTESTCOND (estiOK == result, stiRESULT_ERROR);
		}
	}
	
STI_BAIL:
	
	return;
}

/* \brief Handle the XML command to set text in a SignMail
 * \param pRootNode - the node that contains the TextInSignMail command
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::textInSignMailSet (IXML_Node *rootNode)
{
	stiDEBUG_TOOL (g_stiVRCLServerDebug,
		stiTrace ("CstiVRCLServer::textInSignMailSet Received %s\n", g_szTextInSignMail);
	);

	stiHResult hResult = stiRESULT_SUCCESS;

	std::string nodeName;
	std::string nodeValue;
	IXML_Node *childNode = nullptr;
	std::string signMailText;
	int startSeconds = -1;
	int endSeconds = -1;

	int nodeCount = GetNodeChildCount(rootNode);
	for (int i = 0; i < nodeCount; i++)
	{
		if (childNode)
		{
			hResult =  GetNextChildNodeNameAndValue (childNode, nodeName, nodeValue, &childNode);
			stiTESTRESULT ();
		}
		else
		{
			hResult = GetChildNodeNameAndValue (rootNode, nodeName, nodeValue, &childNode);
			stiTESTRESULT ();
		}

		if (nodeName == g_szSignMailText)
		{
			signMailText.assign(nodeValue);
		}
		else if (nodeName == g_szStartSeconds)
		{
			try
			{
				startSeconds = std::stoi(nodeValue);
			}
			catch (...)
			{
				stiTHROW(stiRESULT_INVALID_PARAMETER);
			}
		}
		else if (nodeName == g_szEndSeconds)
		{
			try
			{
				endSeconds = std::stoi(nodeValue);
			}
			catch (...)
			{
				stiTHROW(stiRESULT_INVALID_PARAMETER);
			}
		}
	}

	textInSignMailSetSignal.Emit (signMailText, 
								  startSeconds, 
								  endSeconds);

STI_BAIL:

	return hResult;
}

void CstiVRCLServer::eventHearingVideoCapable (
	bool hearingVideoCapable)
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventHearingVideoCapable);

	// Hearing Video needs 4.4
	if (ClientVersionCheck (4, 4) &&
	    stiINVALID_SOCKET != m_nClientSocket)
	{
		std::string capable = hearingVideoCapable ? "true" : "false";

		std::ostringstream message;
		message << "<" << g_hearingVideoCapable << ">";
		message << capable;
		message << "</" << g_hearingVideoCapable << ">";

		// Send a message to the Client.
		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND (estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:

	return;
}

void CstiVRCLServer::eventHearingVideoConnected ()
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventHearingVideoConnected);

	// Hearing Video needs 4.4
	if (ClientVersionCheck (4, 4) &&
	    stiINVALID_SOCKET != m_nClientSocket)
	{
		std::ostringstream message;
		message << "<" << g_hearingVideoConnected << " />";

		// Send a message to the Client.
		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND (estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:

	return;
}


void CstiVRCLServer::eventHearingVideoConnectionFailed ()
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventHearingVideoConnectionFailed);

	// Hearing Video needs 4.4
	if (ClientVersionCheck (4, 4) &&
	    stiINVALID_SOCKET != m_nClientSocket)
	{
		std::ostringstream message;
		message << "<" << g_hearingVideoConnectionFailed << " />";

		// Send a message to the Client.
		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND (estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:

	return;
}


void CstiVRCLServer::eventHearingVideoDisconnected (
	bool hearingDisconnected)
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventHearingVideoDisconnected);

	// Hearing Video needs 4.4
	if (ClientVersionCheck (4, 4) &&
	    stiINVALID_SOCKET != m_nClientSocket)
	{
		std::string disconnectedBy = hearingDisconnected ? g_hearingVideoDisconnectedByHearing : g_hearingVideoDisconnectedByDeaf;

		std::ostringstream message;
		message << "<" << g_hearingVideoDisconnected << ">";
		message << disconnectedBy;
		message << "</" << g_hearingVideoDisconnected << ">";

		// Send a message to the Client.
		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND (estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:

	return;
}

void CstiVRCLServer::eventHearingVideoConnecting(
	const std::string &conferenceURI,
	const std::string &initiator,
	const std::string &profileId)
{
	stiHResult hResult = stiRESULT_SUCCESS;  stiUNUSED_ARG(hResult);
	stiLOG_ENTRY_NAME (CstiVRCLServer::eventHearingVideoConnecting);

	// Hearing Video needs 4.4
	if (ClientVersionCheck (4, 4) &&
	    stiINVALID_SOCKET != m_nClientSocket)
	{
		std::ostringstream message;
		message << "<" << g_hearingVideoConnecting << ">";
		message << "<" << g_hearingVideoConnectingConfURI << ">" << conferenceURI << "</" << g_hearingVideoConnectingConfURI << ">";
		message << "<" << g_hearingVideoConnectingInitiator << ">" << initiator << "</" << g_hearingVideoConnectingInitiator << ">";
		message << "<" << g_hearingVideoConnectingProfileId << ">" << profileId << "</" << g_hearingVideoConnectingProfileId << ">";
		message << "</" << g_hearingVideoConnecting << ">";

		// Send a message to the Client.
		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND (estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:

	return;

}

/* \brief Handle the XML command to dial an external conference
 * \param pRootNode - the node that contains the ExternalConferenceDial command
 *
 * \return stiHResult
 */
stiHResult CstiVRCLServer::externalConferenceDial(IXML_Node* pRootNode)
{
	stiDEBUG_TOOL(g_stiVRCLServerDebug,
		stiTrace("CstiVRCLServer::ExternalConferenceDial Received %s\n", g_szExternalConferenceDialMessage);
	);

	/*
	 * <ExternalConferenceDial>
	 * 	<ConferenceURI>https://xxx/xxx/xxx</ConferenceURI>
	 *  <ConferenceType>xxx</ConferenceType>
	 *  <PublicIP>x.x.x.x</PublicIP>
	 * </ExternalConferenceDial>
	 */

	IXML_Node* pNode = pRootNode;
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string conferenceURI{};
	std::string conferenceType{};
	std::string publicIP{};
	std::string securityToken{};
	
	ExternalConferenceData externalConferenceData{};

	// Get number of child elements... to know how many to loop through
	int nCount = GetNodeChildCount(pRootNode);

	for (int i = 0; i < nCount; i++)
	{
		std::string nodeName;
		std::string nodeValue;
		if (i == 0)
		{
			hResult = GetChildNodeNameAndValue(pNode, nodeName, nodeValue, &pNode);
			stiTESTRESULT()
		}
		else
		{
			hResult = GetNextChildNodeNameAndValue(pNode, nodeName, nodeValue, &pNode);
			stiTESTRESULT();
		}

		if (g_szConferenceURI == nodeName)
		{
			conferenceURI = nodeValue;
		}
		else if (g_szConferenceType == nodeName)
		{
			conferenceType = nodeValue;
		}
		else if (g_szPublicIP == nodeName)
		{
			publicIP = nodeValue;
		}
		else if (g_szSecurityToken == nodeName)
		{
			securityToken = nodeValue;
		}
	}

	externalConferenceData = ExternalConferenceData(conferenceURI, conferenceType, publicIP, securityToken);
	externalConferenceSignal.Emit(externalConferenceData);
	m_bCallInProgress = estiTRUE;

STI_BAIL:

	return hResult;
}

stiHResult CstiVRCLServer::externalCameraUse(IXML_Node* pRootNode)
{
	stiDEBUG_TOOL(g_stiVRCLServerDebug,
		stiTrace("CstiVRCLServer::externalCameraUse Received %s\n", g_szExternalCameraUseMessage);
	);

	/*
	 * <ExternalCameraUse>
	 * 	<InUse>true</InUse>
	 * </ExternalCameraUse>
	 */

	IXML_Node* pNode = pRootNode;
	stiHResult hResult = stiRESULT_SUCCESS;

	bool inUse =  false;

	std::string nodeName;
	std::string nodeValue;
	hResult = GetChildNodeNameAndValue(pNode, nodeName, nodeValue, &pNode);
	stiTESTRESULT()

	if (g_szInUse == nodeName)
	{
		std::transform(nodeValue.begin(), nodeValue.end(), nodeValue.begin(), ::tolower);
		inUse = "true" == nodeValue;
	}

	externalCameraUseSignal.Emit(inUse);

STI_BAIL:

	return hResult;
}


void CstiVRCLServer::externalCallSet(VRCLCallSP& externalCall)
{
	if (externalCall != m_externalCall)
	{
		m_bCallInProgress = externalCall != nullptr ? estiTRUE: estiFALSE;
		if (externalCall == nullptr && m_externalCall != nullptr)
		{
			eventCallTerminated(m_externalCall);
		}
		m_externalCall = externalCall;
	}
}

VRCLCallSP CstiVRCLServer::callObjectHeadGet()
{
	if (m_externalCall != nullptr)
	{
		return m_externalCall;
	}

	return m_pConferenceManager->CallObjectHeadGet();
}

VRCLCallSP CstiVRCLServer::callObjectGet(uint32_t stateMask)
{
	if (m_externalCall != nullptr)
	{
		return m_externalCall;
	}

	return m_pConferenceManager->CallObjectGet(stateMask);
}

stiHResult CstiVRCLServer::externalConferenceStateChanged(const std::string& state)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME(CstiVRCLServer::eventHearingVideoConnected);

	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		std::ostringstream message;
		message << g_szExternalConferenceStatus << state << g_szExternalConferenceStatusEnd;

		// Send a message to the Client.
		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND(estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}

stiHResult CstiVRCLServer::externalParticipantCountChanged(int32_t count)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	stiLOG_ENTRY_NAME(CstiVRCLServer::eventHearingVideoConnected);

	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		std::ostringstream message;
		message << g_szExternalConferenceParticipantCount << count << g_szExternalConferenceParticipantCountEnd;

		// Send a message to the Client.
		auto result = ClientResponseSend(message.str().c_str());

		stiTESTCOND(estiOK == result, stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}

// end file CstiVRCLServer.cpp
