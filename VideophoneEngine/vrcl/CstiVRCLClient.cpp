////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiVRCLClient
//
//  File Name:  CstiVRCLClient.cpp
//
//  Abstract:
//      Client for the Videphone Remote Control and Logging facility. This
//      client application connects to a Server to send event notifications
//      about call state, and also to remotely control the videophone.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include "CstiVRCLClient.h"
#include "stiVRCLCommon.h"

#ifdef WIN32
	#include <winsock2.h>
	#include <fcntl.h>
#elif APPLICATION == APP_NTOUCH_ANDROID
	#include <sys/socket.h>
	#include <fcntl.h>
#else
	#include <sys/socket.h>
	#include <sys/fcntl.h>
#endif
#include <cctype>

//#include <ctype.h> // for isdigit
#include "stiBase64.h"
#include "stiTaskInfo.h"
#include "stiClientValidate.h"
#include "stiOS.h"
#include "stiTrace.h"
#include "stiTools.h"
#include "CstiExtendedEvent.h"
#include "CstiUniversalEvent.h"
#include "CstiHostNameResolver.h"
#include "stiVRCLCommon.h"
#include <sstream>
#include <net/if.h>

//
// Constants
//
#define stiMAX_VRCL_READ_SIZE 16384

// Commands/Responses Sent
static const char g_szAddressDial[] = "<AddressDial>";
static const char g_szAddressDialEnd[] = "</AddressDial>";
static const char g_szAudioPrivacySet[] = "<AudioPrivacySet>";
static const char g_szAudioPrivacySetEnd[] = "</AudioPrivacySet>";
static const char g_szAPIVersionGet[] = "<APIVersionGet>";
static const char g_szAPIVersionGetEnd[] = "</APIVersionGet>";
static const char g_szCallBridgeDial[] = "<CallBridgeDial>";
static const char g_szCallBridgeDialEnd[] = "</CallBridgeDial>";
static const char g_szCallDialed[] = "<CallDialed>";
static const char g_szCallTransfer[] = "<CallTransfer>";
static const char g_szCallTransferEnd[] = "</CallTransfer>";
static const char g_szCallTerminated[] = "<CallTerminated>";
static const char g_szConnect[] = "<Connect>";
static const char g_szConnectEnd[] = "</Connect>";
static const char g_szIsAliveAck[] = "<IsAliveAck />";
static const char g_szLocalNameSet[] = "<LocalNameSet>";
static const char g_szLocalNameSetEnd[] = "</LocalNameSet>";
static const char g_szPhoneNumberDial[] = "<PhoneNumberDial>";
static const char g_szPhoneNumberDialEnd[] = "</PhoneNumberDial>";
static const char g_szSettingGet[] = "<SettingGet Type=\"%s\" Name=\"%s\" />";
static const char g_szSettingSet[] = "<SettingSet Type=\"%s\" Name=\"%s\">%s</SettingSet>";
static const char g_szVRSHearingPhoneNumberSet[] = "<VRSHearingPhoneSet>";
static const char g_szVRSHearingPhoneNumberSetEnd[] = "</VRSHearingPhoneSet>";
static const char g_szValidate[] = "<Validate>";
static const char g_szValidateEnd[] = "</Validate>";
static const char g_szVideoPrivacySet[] = "<VideoPrivacySet>";
static const char g_szVideoPrivacySetEnd[] = "</VideoPrivacySet>";
static const char g_szCallHoldSet[] = "CallHoldSet";
static const char g_szCallHoldSetEnd[] = "</CallHoldSet>";
static const char g_szVRSDial[] = "<VRSDial>";
static const char g_szVRSDialEnd[] = "</VRSDial>";
static const char g_szVRSVCODial[] = "<VRSVCODial>";
static const char g_szVRSVCODialEnd[] = "</VRSVCODial>";
static const char g_szUpdateCheckRequest[] = "<UpdateCheck />";
static const char g_szDebugToolSetRequest[] = "<DebugToolSet><Name>%s</Name><Value>%s</Value><Persist>%s</Persist></DebugToolSet>";

// Messages Received
static const char g_szAPIVersionMessage[] = "<APIVersion>";
static const char g_szApplicationVersionMessage[] = "<ApplicationVersion>";
static const char g_szAudioPrivacySetSuccessMessage[] = "<AudioPrivacySetSuccess />";
//static const char g_szAudioPrivacySetFailMessage[] = "<AudioPrivacySetFail />";
static const char g_szAllowedAudioCodecsFailMessage[] = "<AllowedAudioCodecsFail />";
static const char g_szAllowedAudioCodecsSuccessMessage[] = "<AllowedAudioCodecsSuccess />";
static const char g_szCallBandwidthDetectionBeginSuccessMessage[] = "<CallBandwidthDetectionBeginSuccess />";
static const char g_szCallIsTransferableMessage[] = "<CallIsTransferable />";
static const char g_szCallIsNotTransferableMessage[] = "<CallIsNotTransferable />";
static const char g_szCallTransferSuccess[] = "<CallTransferSuccess />";
static const char g_szConnectAckMessage[] = "<ConnectAck />";
static const char g_szConnectAckBeginMessage[] = "<ConnectAck"; // Note:  The end bracket is intentionally missing!
static const char g_szCallConnectedExMessage[] = "<CallConnectedEx>";
static const char g_szCallInProgressMessage[] = "<CallInProgress />";
static const char g_szDeviceTypeMessage[] = "<DeviceType>";
static const char g_szIncomingCallMessage[] = "<IncomingCall />";
static const char g_szOutgoingCallMessage[] = "<OutgoingCall />";
static const char g_szLocalNameSetSuccessMessage[] = "<LocalNameSetSuccess />";
static const char g_szSettingGetMessage[] = "<Setting Type ="; // Note:  The end bracket is intentionally missing!
static const char g_szSettingSetSuccessMessage[] = "<SettingSetSuccess />";
static const char g_szStatusConnectingMessage[] = "<StatusConnecting>";
static const char g_szStatusInCallExMessage[] = "<StatusInCallEx>";
static const char g_szStatusIdleMessage[] = "<StatusIdle />";
static const char g_szValidateAckMessage[] = "<ValidateAck />";
static const char g_szVideoPrivacySetSuccessMessage[] = "<VideoPrivacySetSuccess />";
static const char g_szUpdateCheckResultMessage[] = "<UpdateCheckResult>";
static const char g_szHearingStatusResponseStart[] = "<HearingStatusResponse>";
static const char g_szRelayNewCallReadyResponseStart[] = "<RelayNewCallReadyResponse>";
static const char g_szContactShareStart[] = "<ContactShare>";


static const char g_szOnSwitch[] = "On";
static const char g_szOffSwitch[] = "Off";

#define stiVRCL_CLI_TASK_NAME						"stiVRCLCli"
#define stiVRCL_CLI_TASK_PRIORITY				90
#define stiVRCL_CLI_STACK_SIZE					4096
#define stiVRCL_CLI_MAX_MESSAGES_IN_QUEUE		6
#define stiVRCL_CLI_MAX_MSG_SIZE					512

#define stiREAD_TIMEOUT_VALUE	30	// Read timeout value in seconds

#define VRCL_IO_BUFFER_SIZE 1024

#define VRCL_PORT_DEFAULT	15327


//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//



//
// Event Map
//
stiEVENT_MAP_BEGIN (CstiVRCLClient)
stiEVENT_DEFINE (estiEVENT_ANSWER, CstiVRCLClient::AnswerHandle)
stiEVENT_DEFINE (estiEVENT_APPLICATION_VERSION_GET, CstiVRCLClient::ApplicationVersionGetHandle)
stiEVENT_DEFINE (estiEVENT_AUDIO_PRIVACY_SET, CstiVRCLClient::AudioPrivacySetHandle)
stiEVENT_DEFINE (estiEVENT_CALL_BANDWIDTH_DETECTION_BEGIN, CstiVRCLClient::CallBandwidthDetectionBeginHandle)
stiEVENT_DEFINE (estiEVENT_CONNECT, CstiVRCLClient::ConnectHandle)
stiEVENT_DEFINE (estiEVENT_DEVICE_TYPE_GET, CstiVRCLClient::DeviceTypeGetHandle)
stiEVENT_DEFINE (estiEVENT_DIAL, CstiVRCLClient::DialHandle)
stiEVENT_DEFINE (estiEVENT_DIAL_EX, CstiVRCLClient::DialExHandle)
stiEVENT_DEFINE (estiEVENT_ENABLE_SIGN_MAIL_RECORDING, CstiVRCLClient::EnableSignMailRecordingHandle)
stiEVENT_DEFINE (estiEVENT_HANGUP, CstiVRCLClient::HangupHandle)
stiEVENT_DEFINE (estiEVENT_IS_CALL_TRANSFERABLE, CstiVRCLClient::IsCallTransferableHandle)
stiEVENT_DEFINE (estiEVENT_LOCAL_NAME_SET, CstiVRCLClient::LocalNameSetHandle)
stiEVENT_DEFINE (estiEVENT_LOGGED_IN_PHONE_NUMBER_GET, CstiVRCLClient::LoggedInPhoneNumberGetHandle)
stiEVENT_DEFINE (estiEVENT_TEXT_MESSAGE_SEND, CstiVRCLClient::TextMessageSendHandle)
stiEVENT_DEFINE (estiEVENT_DTMF_MESSAGE_SEND, CstiVRCLClient::DtmfMessageSendHandle)
stiEVENT_DEFINE (estiEVENT_DIAGNOSTICS_GET, CstiVRCLClient::DiagnosticsGetHandle)
stiEVENT_DEFINE (estiEVENT_REBOOT, CstiVRCLClient::RebootHandle)
stiEVENT_DEFINE (estiEVENT_REJECT, CstiVRCLClient::RejectHandle)
stiEVENT_DEFINE (estiEVENT_SETTING_GET, CstiVRCLClient::SettingGetHandle)
stiEVENT_DEFINE (estiEVENT_SETTING_SET, CstiVRCLClient::SettingSetHandle)
stiEVENT_DEFINE (estiEVENT_SHUTDOWN, CstiVRCLClient::ShutdownHandle)
stiEVENT_DEFINE (estiEVENT_SKIP_TO_SIGN_MAIL_RECORD, CstiVRCLClient::SkipToSignMailRecordHandle)
stiEVENT_DEFINE (estiEVENT_STATUS_CHECK, CstiVRCLClient::StatusCheckHandle)
stiEVENT_DEFINE (estiEVENT_TRANSFER, CstiVRCLClient::TransferHandle)
stiEVENT_DEFINE (estiEVENT_VIDEO_PRIVACY_SET, CstiVRCLClient::VideoPrivacySetHandle)
stiEVENT_DEFINE (estiEVENT_CALL_HOLD_SET, CstiVRCLClient::CallHoldSetHandle)
stiEVENT_DEFINE (estiEVENT_VRS_DIAL, CstiVRCLClient::VRSDialHandle)
stiEVENT_DEFINE (estiEVENT_VRS_HEARING_NUMBER_SET, CstiVRCLClient::VRSHearingPhoneNumberSetHandle)
stiEVENT_DEFINE (estiEVENT_VRS_VCO_DIAL, CstiVRCLClient::VRSVCODialHandle)
stiEVENT_DEFINE (estiEVENT_CALL_BRIDGE_DIAL, CstiVRCLClient::CallBridgeDialHandle)
stiEVENT_DEFINE (estiEVENT_CALL_BRIDGE_DISCONNECT, CstiVRCLClient::CallBridgeDisconnectHandle)
stiEVENT_DEFINE (estiEVENT_ALLOWED_AUDIO_CODECS, CstiVRCLClient::AllowedAudioCodecsHandle)
stiEVENT_DEFINE (estiEVENT_UPDATE_CHECK, CstiVRCLClient::UpdateCheckHandle)
stiEVENT_DEFINE (estiEVENT_DEBUG_TOOL_SET, CstiVRCLClient::DebugToolSetHandle)
stiEVENT_DEFINE (estiEVENT_VRS_ANNOUNCE_CLEAR, CstiVRCLClient::AnnounceClearHandle)
stiEVENT_DEFINE (estiEVENT_VRS_ANNOUNCE_SET, CstiVRCLClient::AnnounceSetHandle)
stiEVENT_DEFINE (estiEVENT_TERP_NET_MODE_SET, CstiVRCLClient::TerpNetModeSetHandle)
stiEVENT_DEFINE (estiEVENT_TEXT_BACKSPACE_SEND, CstiVRCLClient::TextBackspaceSendHandle)
stiEVENT_DEFINE (estiEVENT_HEARING_STATUS_CHANGED, CstiVRCLClient::HearingStatusHandle)
stiEVENT_DEFINE (estiEVENT_RELAY_NEW_CALL_READY, CstiVRCLClient::RelayNewCallReadyHandle)
stiEVENT_DEFINE (estiEVENT_TEXT_IN_SIGNMAIL, CstiVRCLClient::textInSignMailHandle)
stiEVENT_DEFINE (estiEVENT_CALL_ID_SET, CstiVRCLClient::callIdSetHandle)
stiEVENT_DEFINE (estiEVENT_HEARING_VIDEO_CAPABILITY, CstiVRCLClient::hearingVideoCapabilityHandle)
stiEVENT_MAP_END (CstiVRCLClient)
stiEVENT_DO_NOW (CstiVRCLClient)

//
// Function Declarations
//

//:-----------------------------------------------------------------------------
//: Construction/Destruction
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiVRCLClient::CstiVRCLClient
//
// Description: Constructor
//
// Abstract:
//
// Returns:
//  None
//
CstiVRCLClient::CstiVRCLClient (
	PstiObjectCallback pfnAppCallback,
	size_t CallbackParam,
	bool bXmlVerbose)
	:
	CstiOsTaskMQ (
		pfnAppCallback,
		CallbackParam,
		(size_t)this,
		stiVRCL_CLI_MAX_MESSAGES_IN_QUEUE,
		stiVRCL_CLI_MAX_MSG_SIZE,
		stiVRCL_CLI_TASK_NAME,
		stiVRCL_CLI_TASK_PRIORITY,
		stiVRCL_CLI_STACK_SIZE),
	m_nClientSocket (stiINVALID_SOCKET),
	m_nServerAPIMajorVersion (0),
	m_nServerAPIMinorVersion (0),
	m_bXmlVerbose (bXmlVerbose),
	m_bRobustCallbacks(estiFALSE),
	m_pxDoc(nullptr),
	m_pRootNode(nullptr)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CstiVRCLClient);
	m_strClientID[0] = '\0';  // initialize to an empty string
} // end CstiVRCLClient::CstiVRCLClient


////////////////////////////////////////////////////////////////////////////////
//; CstiVRCLClient::~CstiVRCLClient
//
// Description: Destructor
//
// Abstract:
//
// Returns:
//  None
//
CstiVRCLClient::~CstiVRCLClient ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::~CstiVRCLClient);

	Shutdown ();

	WaitForShutdown ();

} // end CstiVRCLClient::~CstiVRCLClient



////////////////////////////////////////////////////////////////////////////////
//; VrclWrite
//
// Description: Writes to an open vrcl connection.
//
// Returns:
//	The number of bytes written.
//
stiHResult CstiVRCLClient::VrclWrite (
	const char *buf,
	unsigned long *punLength)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	long nLen = 0;
	
	stiTESTCOND (m_nClientSocket != stiINVALID_SOCKET, stiRESULT_ERROR);
	
#ifndef WIN32
	InitSigPipeHandler ();
#endif

	{
		std::stringstream SendBuf;
		SendBuf << g_szIsAliveMessage << buf;
		nLen = stiOSWrite (m_nClientSocket, SendBuf.str().c_str(), SendBuf.str().size());
		if (nLen == -1)
		{
			perror ("CstiVRCLClient::VrclWrite");
		}
	}
	
#ifndef WIN32
	UninitSigPipeHandler ();
#endif
	
	if (nLen == -1)
	{
		stiDEBUG_TOOL(g_stiVRCLServerDebug,
			stiTrace ("ERR: System error writing to socket err(%d) %s\n", errno, strerror (errno));
		);
		stiOSClose (m_nClientSocket);
		m_nClientSocket = stiINVALID_SOCKET;
		
		stiTESTCOND (nLen != -1, stiRESULT_ERROR);
	}
	
	if (punLength)
	{
		*punLength = nLen;
	}

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; VrclRead
//
// Description: Reads data from an open vrcl connection.
//
// Abstract:
//	User must pass in an allocated returnBuf of at least returnBufSize bytes
//   to be filled in by this function.
//
// Returns:
//	The number of bytes read.
//
stiHResult CstiVRCLClient::VrclRead (
	char *pReturnBuf,
	int nReturnBufSize,
	int *pnBytesRead)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	fd_set SReadFds;
	struct timeval TimeOut{};
	int nSelectRet;
	
	//
	// If the socket isn't open then throw an error.
	//
	stiTESTCOND (m_nClientSocket != stiINVALID_SOCKET, stiRESULT_ERROR);
	
	TimeOut.tv_sec = stiREAD_TIMEOUT_VALUE;
	TimeOut.tv_usec = 0;
	
	stiFD_ZERO (&SReadFds);

	// select on the socket
	FD_SET (m_nClientSocket, &SReadFds);

	nSelectRet = stiOSSelect (FD_SETSIZE, &SReadFds, nullptr, nullptr, &TimeOut);

	if (nSelectRet != -1 && FD_ISSET (m_nClientSocket, &SReadFds))
	{
		int nLen = stiOSRead (m_nClientSocket, pReturnBuf, nReturnBufSize);
		
		if (nLen >= 0)
		{
			pReturnBuf[nLen] = '\0';
			
			if (pnBytesRead)
			{
				*pnBytesRead = nLen;
			}
		}
		else
		{
			stiTHROW (stiRESULT_ERROR);
		}
	}
	else
	{
		stiTHROW (stiRESULT_ERROR);
	}

STI_BAIL:

	return hResult;
}

#ifndef WIN32
stiHResult CstiVRCLClient::InitSigPipeHandler ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int returnVal = 0;
	struct sigaction sigPipe{};
	
	memset (&sigPipe, 0, sizeof(sigPipe));

	sigPipe.sa_handler = SIG_IGN;
	sigemptyset(&sigPipe.sa_mask);
	sigPipe.sa_flags = 0;

	returnVal = sigaction(SIGPIPE, &sigPipe, &m_OldSigHandler);
	stiTESTCOND (returnVal == 0, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}


void CstiVRCLClient::UninitSigPipeHandler ()
{
	sigaction(SIGPIPE, &m_OldSigHandler, nullptr);
}
#endif


stiHResult CstiVRCLClient::Startup ()
{
	m_oMessageBuffer.Open (nullptr, stiMAX_VRCL_READ_SIZE * 2);

	return CstiOsTaskMQ::Startup ();
}


void CstiVRCLClient::ClientIDSet(int nClient)
{
	if(nClient >= 0)
	{
		// nClient is the client id (e.g. "1" for PHONE1)
		nClient = (nClient < nMAX_VRCL_CLIENT_ID) ? nClient : nMAX_VRCL_CLIENT_ID; //std::min (nClient, nMAX_VRCL_CLIENT_ID);
		sprintf(m_strClientID,"PHONE%d: ",nClient);
	}
}

//:-----------------------------------------------------------------------------
//: Static Callback Functions
//:-----------------------------------------------------------------------------

//:-----------------------------------------------------------------------------
//: Basic Task Methods
//:-----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//; CstiVRCLClient::Task
//
// Description: Main Task Loop
//
// Abstract:
//
// Returns:
//  int
//
int CstiVRCLClient::Task ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Task);

//	EstiResult eResult = estiOK;
	EstiBool bShutdown = estiFALSE;
	fd_set SReadFds;
	int nMsgDesc = FileDescriptorGet ();
	struct timeval * pSTimeOut = nullptr;

	// Loop continually reading incoming events until the boolean indicating
	// we should shutdown has been set.
	while (estiFALSE == bShutdown)
	{

		stiFD_ZERO (&SReadFds);

		// select on the message que
		FD_SET (nMsgDesc, &SReadFds);

		// if we have an open socket with a server, select on that socket too
		Lock ();
		if (stiINVALID_SOCKET != m_nClientSocket)
		{
			FD_SET (m_nClientSocket, &SReadFds);
		}
		Unlock ();

		// wait for data to come in on the sockets or message queue
		int nSelectRet = stiOSSelect (FD_SETSIZE, &SReadFds, nullptr, nullptr, pSTimeOut);
		if (stiERROR != nSelectRet)
		{
			// Is the incoming message a request from another task?
			if (0 < nMsgDesc && FD_ISSET (nMsgDesc, &SReadFds))
			{
				//Yes, Handle the message
				// read a message from the message queue
				int szBuffer[(stiVRCL_MAX_MSG_SIZE / sizeof (int) + 1) + 1];

				stiHResult hResult = MsgRecv ((char *)szBuffer, stiVRCL_CLI_MAX_MSG_SIZE);

				if (!stiIS_ERROR (hResult))
				{
					auto  poEvent = (CstiEvent * )(void *)szBuffer;

					// Lookup and handle the event
					Lock ();
					hResult = EventDoNow (poEvent);
					Unlock ();

					// Check and handle the error from EventDoNow.
					if (stiIS_ERROR (hResult))
					{
						// Notify the CCI of this error
						Callback(estiMSG_CB_ERROR_REPORT, (size_t)m_pstErrorLog);
					} // end if

					// Determine the event . . . is it a "shutdown" message?
					if (estiEVENT_SHUTDOWN == poEvent->EventGet ())
					{
						// Yes.  Time to quit.  Set the shutdown flag to true
						bShutdown = estiTRUE;
					} // end if
				} // end if
				else
				{
					// Check and handle the return code from MsgRecv
					stiDEBUG_TOOL(g_stiVRCLServerDebug,
						stiTrace ("%sCstiVRCLClient::Task - MsgRecv returned an error.\n", m_strClientID);
					);
					stiASSERT (estiFALSE);
				} // end else
			} // end if

			Lock ();
			
			// Is the client socket waiting to be read?
			if (stiINVALID_SOCKET != m_nClientSocket && FD_ISSET (m_nClientSocket, &SReadFds))
			{
				char byBuffer[stiMAX_VRCL_READ_SIZE];
				int nBytesRead = 0;
				
				// Yes! Read incoming information
				stiHResult hResult = VrclRead (byBuffer, sizeof (byBuffer), &nBytesRead);

				// Was there any data read from the socket?
				if (!stiIS_ERROR (hResult))
				{
					if (nBytesRead > 0)
					{
						MessageProcess (byBuffer, nBytesRead);
					}
				} // end if
				else
				{
					// No! Close the socket connection.
					stiDEBUG_TOOL(g_stiVRCLServerDebug,
						stiTrace ("%sCstiVRCLServer::Task - Error reading from socket, Closing.\n", m_strClientID);
					);
					stiOSClose (m_nClientSocket);
					m_nClientSocket = stiINVALID_SOCKET;
				} // end else
			}

			// Unlock the mutex
			Unlock ();
		} // end if
	} // end while

	return (0);
} // end CstiVRCLClient::Task


stiHResult CstiVRCLClient::ShutdownHandle (
	CstiEvent * poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::ShutdownHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		stiOSClose (m_nClientSocket);
		m_nClientSocket = stiINVALID_SOCKET;
	}

	// Purge the circular buffer to clear out any old messages.
	m_oMessageBuffer.Purge ();

	// Close the circular message buffer for handling client messages
	m_oMessageBuffer.Close ();

	// Call the base class shutdown.
	CstiOsTaskMQ::ShutdownHandle ();

	return (hResult);
}

stiHResult CstiVRCLClient::AnnounceClear ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AnnouceClear);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_VRS_ANNOUNCE_CLEAR);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}

////////////////////////////////////////////////////////////////////////////////
//; AnnouceClearHandle
//
// Description: Clears the Announce text dialog from the VP.
//
// Abstract:
//	This tells the VP to clear the Announce text dialog.
//
// Returns:
//
//
stiHResult CstiVRCLClient::AnnounceClearHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AnnounceClearHandle);

	char szBuff[255];
	snprintf (szBuff, sizeof (szBuff) - 1, "<%s />", g_szAnnounceClear);
	
	return (VrclWrite (szBuff, nullptr));
}


stiHResult CstiVRCLClient::AnnounceSet (
	const char *pszAnnounceText)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AnnouceSet);
	stiHResult hResult = stiRESULT_SUCCESS;

	if (pszAnnounceText && pszAnnounceText[0])
	{
		auto pszAnnounceStr = new char[strlen (pszAnnounceText) + 1];
		strcpy (pszAnnounceStr, pszAnnounceText);

		CstiExtendedEvent oEvent (estiEVENT_VRS_ANNOUNCE_SET, (stiEVENTPARAM)pszAnnounceStr, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete [] pszAnnounceStr;
			pszAnnounceStr = nullptr;
		}
	}

	return (hResult);
}

////////////////////////////////////////////////////////////////////////////////
//; AnnouceSetHandle
//
// Description: Sets the text and causes the Announce text dialog to appear on VP.
//
// Abstract:
//	This tells the VP to display the Announce text dialog.
//
// Returns:
//
//
stiHResult CstiVRCLClient::AnnounceSetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AnnounceClearHandle);

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszAnnounceText = (char*)(pExtEvent->ParamGet (0));

	char szBuff[300];
	snprintf (szBuff, sizeof (szBuff) - 1, "<%s>%s</%s>", g_szAnnounceSet, pszAnnounceText, g_szAnnounceSet);
	
	if (pszAnnounceText)
	{
		delete [] pszAnnounceText;
		pszAnnounceText = nullptr;
	}
	return (VrclWrite (szBuff, nullptr));
}


stiHResult CstiVRCLClient::Answer ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Answer);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_ANSWER);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; AnswerHandle
//
// Description: Answer the phone.
//
// Abstract:
//	This will cause the remote system to answer the ringing phone.
//
// Returns:
//
//
stiHResult CstiVRCLClient::AnswerHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AnswerHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "%s", g_szCallAnswerMessage);
	
	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiVRCLClient::Dial (const char *pszDialString)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Dial);

	stiHResult hResult = stiRESULT_SUCCESS;
	if (pszDialString && pszDialString[0])
	{
		auto pszDialStr = new char[strlen (pszDialString) + 1];
		strcpy (pszDialStr, pszDialString);
		CstiExtendedEvent oEvent (estiEVENT_DIAL, (stiEVENTPARAM)pszDialStr, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete [] pszDialStr;
			pszDialStr = nullptr;
		}
	}

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; DialHandle
//
// Description: Dial the phone.
//
// Abstract:
//	This will auto-detect a phone number vs an ip address or URI.
//
// Returns:
//
//
stiHResult CstiVRCLClient::DialHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DialHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszDialString = (char*)(pExtEvent->ParamGet (0));
	int i;
	char szBuf[255];
	const char *pszDialType = g_szPhoneNumberDial ;
	const char *pszDialTypeEnd =g_szPhoneNumberDialEnd;

	stiTESTCOND (pszDialString && pszDialString[0], stiRESULT_ERROR);

	// If it's not all valid phone number digits, then switch to address dial
	for (i = 0; pszDialString[i]; ++i)
	{
		if ((pszDialString[i] < '0' || pszDialString[i] > '9') && 
			pszDialString[i]!='-' && pszDialString[i]!='+' && pszDialString[i]!='('  && pszDialString[i]!=')')
		{
			pszDialType = g_szAddressDial;
			pszDialTypeEnd = g_szAddressDialEnd;
			break;
		}
	}

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", pszDialType, pszDialString, pszDialTypeEnd);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();
	

STI_BAIL:

	// Delete the dial string that was allocated by the call to Dial.
	if (pszDialString)
	{
		delete [] pszDialString;
		pszDialString = nullptr;
	}

	return hResult;
}


stiHResult CstiVRCLClient::CallBridgeDial (const char *pszDialString)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallBridgeDial);

	stiHResult hResult = stiRESULT_SUCCESS;
	if (pszDialString && pszDialString[0])
	{
		auto pszDialStr = new char[strlen (pszDialString) + 1];
		strcpy (pszDialStr, pszDialString);
		CstiExtendedEvent oEvent (estiEVENT_CALL_BRIDGE_DIAL, (stiEVENTPARAM)pszDialStr, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));

		if (stiIS_ERROR (hResult))
		{
			delete [] pszDialStr;
			pszDialStr = nullptr;
		}
	}

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; CallBridgeDialHandle
//
// Description: Establish an audio call bridge
//
// Abstract:
//
//
// Returns:
//
//
stiHResult CstiVRCLClient::CallBridgeDialHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallBridgeDialHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszDialString = (char*)(pExtEvent->ParamGet (0));
	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf) - 1, "%s%s%s", g_szCallBridgeDial, pszDialString, g_szCallBridgeDialEnd);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	// Delete the dial string that was allocated by the call to Dial.
	if (pszDialString)
	{
		delete [] pszDialString;
		pszDialString = nullptr;
	}

	return hResult;
}

stiHResult CstiVRCLClient::CallBridgeDisconnect ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallBridgeDisconnect);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_CALL_BRIDGE_DISCONNECT);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}

////////////////////////////////////////////////////////////////////////////////
//; CallBridgeDisconnectHandle
//
// Description: Remove the audio call bridge
//
// Abstract:
//
//
// Returns:
//
//
stiHResult CstiVRCLClient::CallBridgeDisconnectHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallBridgeDisconnectHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szCallBridgeDisconnect, nullptr);
	stiTESTRESULT ();

STI_BAIL:
	return hResult;
}

stiHResult CstiVRCLClient::AllowedAudioCodecs (const char *pszCodecsList)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AllowedAudioCodecs);

	stiHResult hResult = stiRESULT_SUCCESS;
	if (pszCodecsList && pszCodecsList[0])
	{
		auto pszCodecsLst = new char[strlen (pszCodecsList) + 1];
		strcpy (pszCodecsLst, pszCodecsList);
		CstiExtendedEvent oEvent (estiEVENT_ALLOWED_AUDIO_CODECS, (stiEVENTPARAM)pszCodecsLst, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));

		if (stiIS_ERROR (hResult))
		{
			delete [] pszCodecsLst;
			pszCodecsLst = nullptr;
		}
	}

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; AllowedAudioCodecsHandle
//
// Description: Set which codecs we will allow
//
// Abstract:
//
//
// Returns:
//
//
stiHResult CstiVRCLClient::AllowedAudioCodecsHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AllowedAudioCodecsHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszCodecsList = (char*)(pExtEvent->ParamGet (0));
	std::stringstream ss;
	
	ss << '<' << g_szAllowedAudioCodecs << '>';

	for (char *pszCodec = strtok (pszCodecsList, ","); pszCodec != nullptr; pszCodec = strtok (nullptr, ","))
	{
		ss << '<' << pszCodec << " />";
	}
	
	ss << "</" << g_szAllowedAudioCodecs << '>';

	hResult = VrclWrite (ss.str ().c_str (), nullptr);
	stiTESTRESULT ();

STI_BAIL:

	// Delete the dial string that was allocated by the call to Dial.
	if (pszCodecsList)
	{
		delete [] pszCodecsList;
		pszCodecsList = nullptr;
	}

	return hResult;
}


stiHResult CstiVRCLClient::DialEx (
	const char *pszCallId,
	const char *pszTerpId,
	const char *pszHearingNumber,
	const char *pszHearingName,
	const char *pszDestNumber,
	bool bAlwaysAllow)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DialEx);
	
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);
	if (pszCallId && pszCallId[0] &&
		 pszTerpId && pszTerpId[0] &&
		 pszHearingNumber && pszHearingNumber[0] &&
		 pszHearingName && pszHearingName[0] &&
		 pszDestNumber && pszDestNumber[0])
	{
		auto pstCallInfo = new (SstiCallDialInfo);
		
		pstCallInfo->eDialMethod = estiDM_BY_DS_PHONE_NUMBER;
		pstCallInfo->CallId.assign (pszCallId);
		pstCallInfo->TerpId.assign (pszTerpId);
		pstCallInfo->HearingNumber.assign (pszHearingNumber);
		pstCallInfo->hearingName = pszHearingName;
		pstCallInfo->DestNumber.assign (pszDestNumber);
		pstCallInfo->bAlwaysAllow = bAlwaysAllow;
		
		CstiExtendedEvent oEvent (estiEVENT_DIAL_EX, (stiEVENTPARAM)pstCallInfo, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete pstCallInfo;
			pstCallInfo = nullptr;
		}
	}
	
	return (hResult);
}


stiHResult CstiVRCLClient::DialExx (
	const char *pszDestNumber,
	bool bAlwaysAllow)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DialExx);
	
	stiHResult hResult = stiMAKE_ERROR(stiRESULT_ERROR);

	if (pszDestNumber && pszDestNumber[0])
	{
		auto pstCallInfo = new (SstiCallDialInfo);
		
		pstCallInfo->eDialMethod = estiDM_BY_DS_PHONE_NUMBER;
		pstCallInfo->DestNumber.assign (pszDestNumber);
		pstCallInfo->bAlwaysAllow = bAlwaysAllow;
		
		CstiExtendedEvent oEvent (estiEVENT_DIAL_EX, (stiEVENTPARAM)pstCallInfo, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete pstCallInfo;
			pstCallInfo = nullptr;
		}
	}
	
	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; DialExHandle
//
// Description: Dial the phone.
//
// Abstract:
//	
//
// Returns:
//
//
stiHResult CstiVRCLClient::DialExHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DialHandle);
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	char *pszRequestString = nullptr;
	auto pstCallInfo = (SstiCallDialInfo*)(pExtEvent->ParamGet (0));
	stiTESTCOND (pstCallInfo, stiRESULT_ERROR);
	
	// create a new xml document
	
	/* Create the XML document to look like:
	 * <PhoneNumberDial>
	 * 	<CallId>AAA</CallId>
	 * 	<TerpId>BBB</TerpId>
	 * 	<HearingNumber>CCC-CCC-CCCC</HearingNumber>
	 * 	<HearingName>DDD</HearingName>
	 * 	<DestNumber>XXX-XXX-XXXX</DestNumber>
	 * </PhoneNumberDial>
	 */
	
	hResult = InitXmlDocument(&m_pxDoc, &m_pRootNode, (char *)g_szPhoneNumberDialMessage);
	stiTESTRESULT ();

	if (!pstCallInfo->CallId.empty())
	{
		hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
										g_szCallId, pstCallInfo->CallId.c_str ());
		stiTESTRESULT ();

		hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
												g_szTerpId, pstCallInfo->TerpId.c_str ());
		stiTESTRESULT ();
		
		hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
												g_szHearingNumber, pstCallInfo->HearingNumber.c_str ());
		stiTESTRESULT ();
		
		hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
												g_szHearingName, pstCallInfo->hearingName.value_or (std::string ()).c_str ());
		stiTESTRESULT ();
	}
	hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
											  g_szDestNumber, pstCallInfo->DestNumber.c_str ());
	stiTESTRESULT ();
	
	hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
											  g_szAlwaysAllow,
											  pstCallInfo->bAlwaysAllow ? "true" : "false");
	stiTESTRESULT ();
	
	hResult = GetXmlString((IXML_Node *)m_pxDoc, &pszRequestString);
	stiTESTRESULT ();
	
	hResult = VrclWrite (pszRequestString, nullptr);
	stiTESTRESULT ();
	
STI_BAIL:
	if (m_pxDoc)
	{
		CleanupXmlDocument(m_pxDoc);
		m_pxDoc = nullptr;
		m_pRootNode = nullptr;
	}
	
	if (pszRequestString)
	{
		delete []pszRequestString;
		pszRequestString = nullptr;
	}
	
	// Free up the memory allocated to send within the event.
	if (pstCallInfo)
	{
		delete pstCallInfo;
		pstCallInfo = nullptr;
	}
	
	return (hResult);	
}


stiHResult CstiVRCLClient::Connect (
	const char *pszServerIP,
	uint16_t un16Port)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Connect);

	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (pszServerIP && pszServerIP[0], stiRESULT_ERROR);

	// Create block to avoid compiler errors with stiTESTCOND above.
	{
		auto pszServerIPCopy = new char[strlen (pszServerIP) + 1];
		strcpy (pszServerIPCopy, pszServerIP);
		CstiExtendedEvent oEvent (estiEVENT_CONNECT, (stiEVENTPARAM)pszServerIPCopy, un16Port);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete [] pszServerIPCopy;
			pszServerIPCopy = nullptr;
		}
	}

STI_BAIL:

	return (hResult);
}


stiHResult CstiVRCLClient::ConnectHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::ConnectHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszServerIP = (char*)pExtEvent->ParamGet (0);
	auto  un16Port = (uint16_t)pExtEvent->ParamGet (1);
	int nSock;

	stiTESTCOND (pszServerIP &&pszServerIP[0], stiRESULT_ERROR);

	// if we already have an open socket, close it first
	if (stiINVALID_SOCKET != m_nClientSocket)
	{
		stiOSClose (m_nClientSocket);
		m_nClientSocket = stiINVALID_SOCKET;
	}

	// Purge the circular buffer to clear out any old messages.
	m_oMessageBuffer.Purge ();

	nSock = VrclConnect (pszServerIP, un16Port);
	if (stiINVALID_SOCKET != nSock)
	{
		Callback (estiMSG_VRCL_CLIENT_CONNECTED, (size_t)0);
	}
	else
	{
		if(m_bRobustCallbacks)
		{
			Callback (estiMSG_VRCL_CLIENT_CONNECT_FAIL, (size_t)0);
		}
		stiDEBUG_TOOL(g_stiVRCLServerDebug,
			stiTrace ("%s<VRCLClient::ConnectHandle> VrclConnect returned error (%d).\n", m_strClientID, nSock);
		);
	}

STI_BAIL:

	delete [] pszServerIP;

	return (hResult);
}


stiHResult CstiVRCLClient::Hangup ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Hangup);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_HANGUP);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; HangupHandle
//
// Description: Hang up the active call.
//
// Returns:
//	0 upon success.
//
stiHResult CstiVRCLClient::HangupHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::HangupHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szCallHangupMessage, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}



stiHResult CstiVRCLClient::Reject ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Reject);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_REJECT);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; RejectHandle
//
// Description: Reject the incoming call.
//
// Returns:
//	0 upon success.
//
stiHResult CstiVRCLClient::RejectHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::RejectHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szCallHangupMessage, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}



////////////////////////////////////////////////////////////////////////////////
//; ApplicationVersionGet
//
// Description: Get the version of the application on the server
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::ApplicationVersionGet ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::ApplicationVersionGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_APPLICATION_VERSION_GET);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::ApplicationVersionGetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::ApplicationVersionGetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szApplicationVersionGetMessage, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; DeviceTypeGet
//
// Description: Get the server's device type
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::DeviceTypeGet ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DeviceTypeGet);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_DEVICE_TYPE_GET);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::DeviceTypeGetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DeviceTypeGetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szDeviceTypeGetMessage, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; IsCallTransferable
//
// Description: Is the current call transferable?
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::IsCallTransferable ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::IsCallTransferable);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_IS_CALL_TRANSFERABLE);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::IsCallTransferableHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::IsCallTransferableHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szIsCallTransferableMessage, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; Reboot
//
// Description: Cause the remote device to reboot
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::Reboot ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Reboot);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_REBOOT);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::RebootHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::RebootHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szRebootMessage, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; StatusCheck
//
// Description: Get the current status
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::StatusCheck ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::StatusCheck);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_STATUS_CHECK);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::StatusCheckHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::StatusCheckHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szStatusCheckMessage, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; CallBandwidthDetectionBegin
//
// Description: Cause the remote device to restart bandwidth detection
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::CallBandwidthDetectionBegin ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallBandwidthDetectionBegin);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_CALL_BANDWIDTH_DETECTION_BEGIN);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::CallBandwidthDetectionBeginHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallBandwidthDetectionBeginHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szCallBandwidthDetectionBegin, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; AudioPrivacySet
//
// Description: Set the server's audio privacy
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::AudioPrivacySet (EstiSwitch eSwitch)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AudioPrivacySet);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_AUDIO_PRIVACY_SET, (stiEVENTPARAM)eSwitch);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::AudioPrivacySetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::AudioPrivacySetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto  eSwitch = (EstiSwitch)poEvent->ParamGet();

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szAudioPrivacySet,
				 estiON == eSwitch ? g_szOnSwitch : g_szOffSwitch, g_szAudioPrivacySetEnd);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();
	

STI_BAIL:

	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; CallHoldSet
//
// Description: Set the server's call hold / resume
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::CallHoldSet (EstiSwitch eSwitch)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallHoldSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_CALL_HOLD_SET, (stiEVENTPARAM)eSwitch);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::CallHoldSetHandle(CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::CallHoldSetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto  eSwitch = (EstiSwitch)poEvent->ParamGet();

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szCallHoldSet,
				 estiON == eSwitch ? g_szOnSwitch : g_szOffSwitch, g_szCallHoldSetEnd);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; VideoPrivacySet
//
// Description: Set the server's video privacy
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::VideoPrivacySet (EstiSwitch eSwitch)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VideoPrivacySet);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_VIDEO_PRIVACY_SET, (stiEVENTPARAM)eSwitch);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::VideoPrivacySetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VideoPrivacySetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto  eSwitch = (EstiSwitch)poEvent->ParamGet();

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szVideoPrivacySet,
				 estiON == eSwitch ? g_szOnSwitch : g_szOffSwitch, g_szVideoPrivacySetEnd);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; LocalNameSet
//
// Description: Set the server's local name
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::LocalNameSet (const char *pszName)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::LocalNameSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszLocalName = new char[strlen(pszName) + 1]; // Needs to be deleted by the handler.
	strcpy (pszLocalName, pszName);

	CstiEvent oEvent (estiEVENT_LOCAL_NAME_SET, (stiEVENTPARAM)pszLocalName);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::LocalNameSetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::LocalNameSetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszName = (char*)poEvent->ParamGet();

	stiTESTCOND (pszName, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szLocalNameSet,
				 pszName, g_szLocalNameSetEnd);

	delete [] pszName;

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiVRCLClient::LoggedInPhoneNumberGet ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::LoggedInPhoneNumberGet);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiEvent oEvent (estiEVENT_LOGGED_IN_PHONE_NUMBER_GET);
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	
	return hResult;
}


stiHResult CstiVRCLClient::LoggedInPhoneNumberGetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::LoggedInPhoneNumberGetHandle);
	
	char szBuff[255];
	snprintf (szBuff, sizeof (szBuff) - 1, "<%s />", g_szLoggedInPhoneNumberGet);
	
	return (VrclWrite (szBuff, nullptr));
}


////////////////////////////////////////////////////////////////////////////////
//; TextBackspaceSend
//
// Description: Cause the remote device backspace one character
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::TextBackspaceSend ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TextBackspaceSend);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_TEXT_BACKSPACE_SEND);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::TextBackspaceSendHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TextBackspaceSendHandle);

	char szBuff[255];
	snprintf (szBuff, sizeof (szBuff) - 1, "<%s />", g_szTextBackspaceSend);

	return (VrclWrite (szBuff, nullptr));
}


stiHResult CstiVRCLClient::TextMessageSend (const char *pszText)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TextMessageSend);

	stiHResult hResult = stiRESULT_SUCCESS;
	char *pszTextCopy = nullptr;

	stiTESTCOND (pszText && *pszText, stiRESULT_INVALID_PARAMETER);

	pszTextCopy = new char[strlen (pszText) + 1];
	strcpy (pszTextCopy, pszText);

	{
		CstiExtendedEvent oEvent (estiEVENT_TEXT_MESSAGE_SEND, (stiEVENTPARAM)pszTextCopy, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
	}
	stiTESTRESULT ();

	// It is sent so we are no longer the owner.
	pszTextCopy = nullptr;

STI_BAIL:

	if (pszTextCopy)
	{
		delete[] pszTextCopy;
		pszTextCopy = nullptr;
	}

	return hResult;
}


stiHResult CstiVRCLClient::TextMessageSendHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TransferHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszText = (char*)(pExtEvent->ParamGet (0));

	unsigned long nLen = 1 + strlen (g_szTextMessageSend) + 1 + strlen (pszText) + 2 + strlen (g_szTextMessageSend) + 1;
	auto pszOutBuf = new char[nLen + 1];

	sprintf (pszOutBuf, "<%s>%s</%s>", g_szTextMessageSend, pszText, g_szTextMessageSend);
	
	hResult = VrclWrite (pszOutBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:
	
	delete[] pszText;
	delete[] pszOutBuf;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; DtmfMessageSend
//
// Description: Send a DTMF character to the VRCL Server.
//
// Abstract: Post an event to the VRCL Client task to send a DTMF character
//	to the VRCL server to be sent by the endpoint.
//
// Returns:
//	an hResult (stiRESULT_SUCCESS for success).
//
stiHResult CstiVRCLClient::DtmfMessageSend (const char c)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DtmfMessageSend);

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiExtendedEvent oEvent (estiEVENT_DTMF_MESSAGE_SEND, (stiEVENTPARAM)c, 0);
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; DtmfMessageSendHandle
//
// Description: Send a DTMF character to the VRCL Server.
//
// Abstract: Valid characters are 0-9, # and *.  Note that we don't validate
//   the characters here.  The server will just drop any invalid characters
//
//
// Returns:
//	an hResult (stiRESULT_SUCCESS for success).
//
stiHResult CstiVRCLClient::DtmfMessageSendHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TransferHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto  c = (char)(pExtEvent->ParamGet (0));

	std::string Buffer = std::string ("<") + g_szDtmfSend + ">" + c + "</" + g_szDtmfSend + ">";

	hResult = VrclWrite (Buffer.c_str (), nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


stiHResult CstiVRCLClient::DiagnosticsGet ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DiagnosticsGet);

	stiHResult hResult = stiRESULT_SUCCESS;
	CstiEvent oEvent (estiEVENT_DIAGNOSTICS_GET);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return hResult;
}


stiHResult CstiVRCLClient::DiagnosticsGetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TransferHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	unsigned long nLen = strlen (g_szDiagnosticsGet) + 3; // Account for the addition of the '/' character and the LT and GT characters.
	auto pszOutBuf = new char[nLen + 1];

	sprintf (pszOutBuf, "<%s/>", g_szDiagnosticsGet);

	hResult = VrclWrite (pszOutBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	delete[] pszOutBuf;

	return hResult;
}


stiHResult CstiVRCLClient::Transfer (const char *pszDialString)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::Transfer);

	stiHResult hResult = stiRESULT_SUCCESS;
	if (pszDialString && pszDialString[0])
	{
		auto pszDialStr = new char[strlen (pszDialString) + 1];
		strcpy (pszDialStr, pszDialString);
		CstiExtendedEvent oEvent (estiEVENT_TRANSFER, (stiEVENTPARAM)pszDialStr, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete [] pszDialStr;
			pszDialStr = nullptr;
		}
	}

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; TransferHandle
//
// Description: Transfer the call.
//
// Abstract:
//	This will transfer the call connected to another device.
//
// Returns:
//
//
stiHResult CstiVRCLClient::TransferHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TransferHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszDialString = (char*)(pExtEvent->ParamGet (0));
	char szBuf[255];

	stiTESTCOND (pszDialString && pszDialString[0], stiRESULT_ERROR);

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szCallTransfer, pszDialString, g_szCallTransferEnd);
	
	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	// Delete the dial string that was allocated by the call to transfer to.
	delete [] pszDialString;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; VRSHearingPhoneNumberSet
//
// Description: Set the VRS hearing number
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::VRSHearingPhoneNumberSet (const char *pszNumber)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VRSHearingPhoneNumberSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszHearingNumber = new char[strlen(pszNumber) + 1]; // Needs to be deleted by the handler.
	strcpy (pszHearingNumber, pszNumber);

	CstiEvent oEvent (estiEVENT_VRS_HEARING_NUMBER_SET, (stiEVENTPARAM)pszHearingNumber);
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	stiTESTRESULT ();
	
	pszHearingNumber = nullptr;
	
STI_BAIL:

	if (pszHearingNumber)
	{
		delete [] pszHearingNumber;
		pszHearingNumber = nullptr;
	}

	return (hResult);
}


stiHResult CstiVRCLClient::VRSHearingPhoneNumberSetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VRSHearingPhoneNumberSetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszNumber = (char*)poEvent->ParamGet();

	stiTESTCOND (pszNumber, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szVRSHearingPhoneNumberSet,
				 pszNumber, g_szVRSHearingPhoneNumberSetEnd);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	delete [] pszNumber;
	
	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; SettingGet
//
// Description: Get a property value from the remote device
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::SettingGet (const char *pszType, const char *pszName)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::SettingGet);

	stiHResult hResult = stiRESULT_SUCCESS;
	char *pszSettingType = nullptr;
	char *pszSettingName = nullptr;
	
	pszSettingType = new char[strlen(pszType) + 1]; // Needs to be deleted by the handler.
	strcpy (pszSettingType, pszType);

	pszSettingName = new char[strlen(pszName) + 1]; // Needs to be deleted by the handler.
	strcpy (pszSettingName, pszName);

	CstiExtendedEvent oEvent (estiEVENT_SETTING_GET,
							  (stiEVENTPARAM)pszSettingType,
							  (stiEVENTPARAM)pszSettingName);
	
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	stiTESTRESULT ();
	
	pszSettingType = nullptr;
	pszSettingName = nullptr;

STI_BAIL:

	if (pszSettingType)
	{
		delete [] pszSettingType;
		pszSettingType = nullptr;
	}
	
	if (pszSettingName)
	{
		delete [] pszSettingName;
		pszSettingName = nullptr;
	}
	
	return (hResult);
}


stiHResult CstiVRCLClient::SettingGetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::SettingGetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto poUEvent = (CstiExtendedEvent*)poEvent;

	auto pszType = (char*)poUEvent->ParamGet(0);
	auto pszName = (char*)poUEvent->ParamGet(1);

	stiTESTCOND (pszType && pszName, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, g_szSettingGet, pszType, pszName);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	delete [] pszType;
	delete [] pszName;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; SettingSet
//
// Description: Set a property value on the remote device
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::SettingSet (const char *pszType, const char *pszName, const char *pszValue)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::SettingSet);

	stiHResult hResult = stiRESULT_SUCCESS;
	char *pszSettingType = nullptr;
	char *pszSettingName = nullptr;
	char *pszSettingValue = nullptr;
	
	pszSettingType = new char[strlen(pszType) + 1]; // Needs to be deleted by the handler.
	strcpy (pszSettingType, pszType);

	pszSettingName = new char[strlen(pszName) + 1]; // Needs to be deleted by the handler.
	strcpy (pszSettingName, pszName);

	pszSettingValue = new char[strlen(pszValue) + 1]; // Needs to be deleted by the handler.
	strcpy (pszSettingValue, pszValue);

	CstiUniversalEvent oEvent (estiEVENT_SETTING_SET,
										(stiEVENTPARAM)pszSettingType,
										(stiEVENTPARAM)pszSettingName,
										(stiEVENTPARAM)pszSettingValue);
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	stiTESTRESULT ();
	
	pszSettingType = nullptr;
	pszSettingName = nullptr;
	pszSettingValue = nullptr;
	
STI_BAIL:

	if (pszSettingType)
	{
		delete [] pszSettingType;
		pszSettingType = nullptr;
	}

	if (pszSettingName)
	{
		delete [] pszSettingName;
		pszSettingName = nullptr;
	}

	if (pszSettingValue)
	{
		delete [] pszSettingValue;
		pszSettingValue = nullptr;
	}

	return (hResult);
}


stiHResult CstiVRCLClient::SettingSetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::SettingSetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto poUEvent = (CstiUniversalEvent*)poEvent;
	auto pszType = (char*)poUEvent->ParamGet(0);
	auto pszName = (char*)poUEvent->ParamGet(1);
	auto pszValue = (char*)poUEvent->ParamGet(2);

	stiTESTCOND (pszType && pszName && pszValue, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, g_szSettingSet, pszType, pszName, pszValue);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	delete [] pszType;
	delete [] pszName;
	delete [] pszValue;

	return hResult;
}



stiHResult CstiVRCLClient::VRSDial (const char *pszDialString)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VRSDial);

	stiHResult hResult = stiRESULT_SUCCESS;

	if (pszDialString && pszDialString[0])
	{
		auto pszDialStr = new char[strlen (pszDialString) + 1];
		strcpy (pszDialStr, pszDialString);
		CstiExtendedEvent oEvent (estiEVENT_VRS_DIAL, (stiEVENTPARAM)pszDialStr, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete [] pszDialStr;
			pszDialStr = nullptr;
		}
	}

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; VRSDialHandle
//
// Description: Dial the hearing phone using a VRS interpreter.
//
// Abstract:
//
// Returns:
//
//
stiHResult CstiVRCLClient::VRSDialHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VRSDialHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszDialString = (char*)(pExtEvent->ParamGet (0));
	char szBuf[255];

	stiTESTCOND (pszDialString && pszDialString[0], stiRESULT_ERROR);

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szVRSDial, pszDialString, g_szVRSDialEnd);
	
	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	// Delete the dial string that was allocated by the call to Dial.
	delete [] pszDialString;

	return hResult;
}


stiHResult CstiVRCLClient::VRSVCODial (const char *pszDialString)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VRSVCODial);

	stiHResult hResult = stiRESULT_SUCCESS;
	if (pszDialString && pszDialString[0])
	{
		auto pszDialStr = new char[strlen (pszDialString) + 1];
		strcpy (pszDialStr, pszDialString);
		CstiExtendedEvent oEvent (estiEVENT_VRS_VCO_DIAL, (stiEVENTPARAM)pszDialStr, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
		
		if (stiIS_ERROR (hResult))
		{
			delete [] pszDialStr;
			pszDialStr = nullptr;
		}
	}

	return (hResult);
}


////////////////////////////////////////////////////////////////////////////////
//; VRSVCODialHandle
//
// Description: Dial the hearing phone using a VRS interpreter.
//
// Abstract:
//
// Returns:
//
//
stiHResult CstiVRCLClient::VRSVCODialHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::VRSVCODialHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto pszDialString = (char*)(pExtEvent->ParamGet (0));
	char szBuf[255];

	stiTESTCOND (pszDialString && pszDialString[0], stiRESULT_ERROR);

	snprintf (szBuf, sizeof (szBuf)-1, "%s%s%s", g_szVRSVCODial, pszDialString, g_szVRSVCODialEnd);
	
	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	// Delete the dial string that was allocated by the call to Dial.
	delete [] pszDialString;

	return hResult;
}


stiHResult CstiVRCLClient::EnableSignMailRecording (int bEnabled)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::EnableSignMailRecording);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiEvent oEvent (estiEVENT_ENABLE_SIGN_MAIL_RECORDING, (size_t)bEnabled);
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	
	return (hResult);
}


stiHResult CstiVRCLClient::EnableSignMailRecordingHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::EnableSignMailRecordingHandle);
	
	auto  bEnabled = (bool)poEvent->ParamGet ();
	
	char szBuff[255];
	snprintf (szBuff, sizeof (szBuff) - 1, "<%s>%s</%s>", g_szEnableSignMailRecording, bEnabled ? "true" : "false", g_szEnableSignMailRecording);
	
	return (VrclWrite (szBuff, nullptr));
}


stiHResult CstiVRCLClient::SkipToSignMailRecord ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::SkipToSignMailRecord);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	CstiEvent oEvent (estiEVENT_SKIP_TO_SIGN_MAIL_RECORD);
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	
	return hResult;
}


stiHResult CstiVRCLClient::SkipToSignMailRecordHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::SkipToSignMailRecordHandle);
	
	char szBuff[255];
	snprintf (szBuff, sizeof (szBuff) - 1, "<%s />", g_szSkipToSignMailRecord);
	
	return (VrclWrite (szBuff, nullptr));
}


////////////////////////////////////////////////////////////////////////////////
//; UpdateCheck
//
// Description: Cause the remote device to check for a version update
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::UpdateCheck ()
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::UpdateCheck);

	stiHResult hResult = stiRESULT_SUCCESS;

	CstiEvent oEvent (estiEVENT_UPDATE_CHECK);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::UpdateCheckHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::UpdateCheckHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	hResult = VrclWrite (g_szUpdateCheckRequest, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//
//
// Description: Cause the remote device to set a debug trace flag value
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::DebugToolSet(const char *pszName, const char *pzsValue, const char *pzsPersist)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DebugToolSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	char *pszTraceName = nullptr;
	char *pszTraceValue = nullptr;
	char *pszTracePersist = nullptr;

	pszTraceName = new char[strlen(pszName) + 1]; // Needs to be deleted by the handler.
	strcpy (pszTraceName, pszName);

	pszTraceValue = new char[strlen(pzsValue) + 1]; // Needs to be deleted by the handler.
	strcpy (pszTraceValue, pzsValue);

	pszTracePersist = new char[2]; //Only need space for one char - Needs to be deleted by the handler.

	if (nullptr != pzsPersist && 0 == stricmp(pzsPersist, "persist"))
	{
		strcpy (pszTracePersist, "1");
	}
	else
	{
		strcpy (pszTracePersist, "0");
	}

	CstiUniversalEvent oEvent (estiEVENT_DEBUG_TOOL_SET,
										(stiEVENTPARAM)pszTraceName,
										(stiEVENTPARAM)pszTraceValue,
										(stiEVENTPARAM)pszTracePersist);
	hResult = MsgSend (&oEvent, sizeof (oEvent));
	stiTESTRESULT ();

	pszTraceName = nullptr;
	pszTraceValue = nullptr;
	pszTracePersist = nullptr;

STI_BAIL:

	if (pszTraceName)
	{
		delete [] pszTraceName;
		pszTraceName = nullptr;
	}

	if (pszTraceValue)
	{
		delete [] pszTraceValue;
		pszTraceValue = nullptr;
	}

	if (pszTracePersist)
	{
		delete [] pszTracePersist;
		pszTracePersist = nullptr;
	}

	return (hResult);
}

stiHResult CstiVRCLClient::DebugToolSetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::DebugToolSetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto poUEvent = (CstiUniversalEvent*)poEvent;
	auto pszTraceName = (char*)poUEvent->ParamGet(0);
	auto pszTraceValue = (char*)poUEvent->ParamGet(1);
	auto pszTracePersist = (char*)poUEvent->ParamGet(2);

	stiTESTCOND (pszTraceName && pszTraceValue, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, g_szDebugToolSetRequest, pszTraceName, pszTraceValue, pszTracePersist);

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	delete [] pszTraceName;
	delete [] pszTraceValue;
	delete [] pszTracePersist;

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; TerpNetModeSet
//
// Description: Set the terp net mode
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::TerpNetModeSet (const char *pszMode)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TerpNetModeSet);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszLocalMode = new char[strlen(pszMode) + 1]; // Needs to be deleted by the handler.
	strcpy (pszLocalMode, pszMode);

	CstiEvent oEvent (estiEVENT_TERP_NET_MODE_SET, (stiEVENTPARAM)pszLocalMode);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::TerpNetModeSetHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TerpNetModeSetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszMode = (char*)poEvent->ParamGet();

	stiTESTCOND (pszMode, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "<%s>%s</%s>", g_szTerpNetModeSet,
			pszMode, g_szTerpNetModeSet);

	delete [] pszMode;

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}



////////////////////////////////////////////////////////////////////////////////
//; HearingStatus
//
// Description: Send the HearingStatus message
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::HearingStatus (const char *pszStatus)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::HearingStatus);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszLocalStatus = new char[strlen(pszStatus) + 1]; // Needs to be deleted by the handler.
	strcpy (pszLocalStatus, pszStatus);

	CstiEvent oEvent (estiEVENT_HEARING_STATUS_CHANGED, (stiEVENTPARAM)pszLocalStatus);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::HearingStatusHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::HearingStatusHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszStatus = (char*)poEvent->ParamGet();

	stiTESTCOND (pszStatus, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "<%s>%s</%s>", g_szHearingStatus,
			pszStatus, g_szHearingStatus);

	delete [] pszStatus;

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}


////////////////////////////////////////////////////////////////////////////////
//; RelayNewCallReady
//
// Description: Send the RelayNewCallReady message
//
// Returns:
//	stiRESULT_SUCCESS or stiRESULT_ERROR.
//
stiHResult CstiVRCLClient::RelayNewCallReady (const char *pszStatus)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::RelayNewCallReady);

	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszLocalStatus = new char[strlen(pszStatus) + 1]; // Needs to be deleted by the handler.
	strcpy (pszLocalStatus, pszStatus);

	CstiEvent oEvent (estiEVENT_RELAY_NEW_CALL_READY, (stiEVENTPARAM)pszLocalStatus);
	hResult = MsgSend (&oEvent, sizeof (oEvent));

	return (hResult);
}


stiHResult CstiVRCLClient::RelayNewCallReadyHandle (CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::RelayNewCallReadyHandle);
	stiHResult hResult = stiRESULT_SUCCESS;

	auto pszStatus = (char*)poEvent->ParamGet();

	stiTESTCOND (pszStatus, stiRESULT_ERROR);

	char szBuf[255];

	snprintf (szBuf, sizeof (szBuf)-1, "<%s>%s</%s>", g_szRelayNewCallReady,
			pszStatus, g_szRelayNewCallReady);

	delete [] pszStatus;

	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}



///////////////////////////////////////////////////////////////////////////////
//
//  stiConnectWithTimeout
//
//  Everyone hates the fact there is no timeout in connect. This function fixes
//  that. With stiConnect you can set that timeout.
//
//
stiHResult stiConnectWithTimeout (int nSfd, struct sockaddr *pAddr, int nAddrlen, int nSeconds)
{
	struct timeval sTimeout{};
	fd_set sFdset;
#ifndef WIN32
	int opts;
#endif
	int nReturn;
	stiHResult hResult = stiRESULT_SUCCESS;

	sTimeout.tv_sec = nSeconds;
	sTimeout.tv_usec = 0;

	if (nSeconds <= 0 )
	{
		nReturn = connect (nSfd, pAddr, nAddrlen);
		stiTESTCOND (nReturn == 0, stiRESULT_DNS_COULD_NOT_RESOLVE)
	}
	else
	{
#ifndef WIN32
		// save flags before setting to non-blocking
		opts = fcntl(nSfd,F_GETFL);
		fcntl(nSfd, F_SETFL, O_NONBLOCK);
#else
		u_long ulNonBlockMode = 1;
		ioctlsocket(nSfd, FIONBIO, &ulNonBlockMode);
#endif

		// the return here is never interesting because it returns before open
		connect (nSfd, pAddr, nAddrlen);

		// setup to call select
		FD_ZERO(&sFdset);
		FD_SET(nSfd, &sFdset);

		// select will timeout according to our parameters
		nReturn = select(nSfd + 1, nullptr, &sFdset, nullptr, &sTimeout);

#ifndef WIN32		// restore the flags
		fcntl(nSfd, F_SETFL, opts);
#endif

		// we expect a one if select was successful before timeout
		stiTESTCOND (nReturn > 0, stiRESULT_DNS_COULD_NOT_RESOLVE)
	}

STI_BAIL:
	return hResult;
}
//Android has problems trying to use ifaddrs so we need to use the old method.
#if APPLICATION != APP_NTOUCH_ANDROID
////////////////////////////////////////////////////////////////////////////////
//; VrclConnect
//
// Description: Establish a remote VRCL connection.
//
// Returns:
//	A handle to the new connection or 0 upon failure.
//
int CstiVRCLClient::VrclConnect (
	const char *pszRemoteHost,
	uint16_t un16Port)
{
	
	stiHResult hResult = stiRESULT_SUCCESS;
	
	char szBuf[VRCL_IO_BUFFER_SIZE];
	addrinfo* pstAddrInfo = nullptr;
	CstiHostNameResolver *poResolver = CstiHostNameResolver::getInstance ();

	// If the port specified is zero then use the standard VRCL port
	if (un16Port == 0)
	{
		un16Port = VRCL_PORT_DEFAULT;
	}

	hResult = poResolver->Resolve (pszRemoteHost, nullptr, &pstAddrInfo);
	stiTESTRESULT ();

	// iterate the list of results
	for (; pstAddrInfo; pstAddrInfo = pstAddrInfo->ai_next)
	{
		// set the port
		if (pstAddrInfo->ai_family == AF_INET)
		{
			((sockaddr_in*)pstAddrInfo->ai_addr)->sin_port = htons (un16Port);
		}
#ifdef IPV6_ENABLED
		else if (pstAddrInfo->ai_family == AF_INET6)
		{
			((sockaddr_in6*)pstAddrInfo->ai_addr)->sin6_port = htons (un16Port);

			// TODO:  This approach is less-than-uninversal
			((sockaddr_in6*)pstAddrInfo->ai_addr)->sin6_scope_id = if_nametoindex ("eth0");
		}
#endif // IPV6_ENABLED
		else
		{
			stiASSERT (false);
			continue;
		}
		
		// Assign the correct protocol
		// (NOTE: We could use the constant IPPROTO_TCP, but this is considered more universal)
		pstAddrInfo->ai_protocol = getprotobyname ("tcp")->p_proto;
		endprotoent ();

		// Start connecting!
		m_nClientSocket = stiOSSocket (pstAddrInfo->ai_family, SOCK_STREAM, pstAddrInfo->ai_protocol); // TODO: may want SOCK_DGRAM (chunks) instead
		if (m_nClientSocket == -1)
		{
			stiTrace ("ERR: Unable to create socket \"%s\" errno(%d)=%s\n", pszRemoteHost, errno, strerror (errno));
			m_nClientSocket = stiINVALID_SOCKET;
			continue;
		}
		
		//hResult = stiConnectWithTimeout (m_nClientSocket, pstAddrInfo->ai_addr, pstAddrInfo->ai_addrlen, nMAX_CONNECT_TIMEOUT);
		if (0!=connect (m_nClientSocket, pstAddrInfo->ai_addr, pstAddrInfo->ai_addrlen))
		//if (stiIS_ERROR(hResult))
		{
			stiTrace ("ERR: System error connecting \"%s\" errno(%d)=%s\n", pszRemoteHost, errno, strerror (errno));
			stiOSClose (m_nClientSocket);
			m_nClientSocket = stiINVALID_SOCKET;
			continue;
		}
		// This address successfully connected so break out of the loop
		break;
	}
	stiTESTCOND (m_nClientSocket != stiINVALID_SOCKET, stiRESULT_ERROR);
	stiTESTCOND (pstAddrInfo, stiRESULT_ERROR);

	// We've got a network connection, now to log in...
	char key[MAX_KEY];

	if (pstAddrInfo->ai_family == AF_INET)
	{
		std::string LocalIP;
		stiGetLocalIp (&LocalIP, estiTYPE_IPV4);

		uint32_t anLocalIp[1];
		inet_pton (pstAddrInfo->ai_family, LocalIP.c_str (), anLocalIp);

		stiKeyCreate (
			pstAddrInfo->ai_family,
			(uint32_t*)&((sockaddr_in*)pstAddrInfo->ai_addr)->sin_addr,
			anLocalIp,
			key);
	}
#ifdef IPV6_ENABLED
	else if (pstAddrInfo->ai_family == AF_INET6)
	{		
		std::string LocalIP;
		stiGetLocalIp (&LocalIP, estiTYPE_IPV6);

		uint32_t anLocalIp[4];
		inet_pton (pstAddrInfo->ai_family, LocalIP.c_str (), anLocalIp);

		stiKeyCreate (
			pstAddrInfo->ai_family, 
			(uint32_t*)&((sockaddr_in6*)pstAddrInfo->ai_addr)->sin6_addr,
			anLocalIp,
			key);
	}
#endif // IPV6_ENABLED
	else
	{
		stiASSERT (estiFALSE);
	}
	
	// Send the connect request
	sprintf (szBuf, "%s%s%s", g_szConnect, key, g_szConnectEnd);
	
	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();

	// Get the result
	hResult = VrclRead (szBuf, sizeof (szBuf), nullptr);
	stiTESTRESULT ();

	// Split this into 2 strings: the xml tag, and the stuff inside it (if any);
	char *pszInnards;
	pszInnards = szBuf;
	for (; *pszInnards; ++pszInnards)
	{
		if (*pszInnards == '>')
		{
			*pszInnards='\0';
			++pszInnards;
			for (int i = 0; pszInnards[i]; ++i) // break at the next tag
			{
				if (pszInnards[i] == '<')
				{
					pszInnards[i]='\0';
					break;
				}
			}
			break;
		}
	}

	// Authenticate
	if (0 == strcmp (szBuf, g_szConnectAckMessage))
	{
		// simple authentication (so we're good)
	}
	else if (0 == strcmp (szBuf, g_szConnectAckBeginMessage))
	{
		char szPassword[MAX_PASSWORD];

		stiPasswordCreate (key, pszInnards, szPassword);

		sprintf (szBuf, "%s%s%s", g_szValidate, szPassword, g_szValidateEnd);
		
		hResult = VrclWrite (szBuf, nullptr);
		stiTESTRESULT ();

		// Get the result
		hResult = VrclRead (szBuf, sizeof (szBuf), nullptr);
		stiTESTRESULT ();

		// Did it work?
		if(0 != strcmp (szBuf, g_szValidateAckMessage))
		{
			stiTrace ("ERR: Authentication failed.\n\t%s", szBuf);
			stiOSClose (m_nClientSocket);
			m_nClientSocket = stiINVALID_SOCKET;
			stiTHROW (stiRESULT_ERROR);
		}
	}

	// Make sure the client knows what vrcl version we'll be using
	sprintf (szBuf, "%s%s%s", g_szAPIVersionGet, g_szAPIVersion, g_szAPIVersionGetEnd);
	
	hResult = VrclWrite (szBuf, nullptr);
	stiTESTRESULT ();
	
	hResult = VrclRead (szBuf, sizeof (szBuf), nullptr);
	stiTESTRESULT ();

	//
	// <APIVersion>
	//
	if (0 == strncmp (
		szBuf,
		g_szAPIVersionMessage,
		sizeof (g_szAPIVersionMessage) - 1))
	{
		// Grab the API version sent by the server.
		char *pszVersion = szBuf + (sizeof (g_szAPIVersionMessage) - 1);

		// Store the API version sent by the server.
		sscanf (
			pszVersion,
			"%d.%d",
			&m_nServerAPIMajorVersion,
			&m_nServerAPIMinorVersion);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (m_nClientSocket != stiINVALID_SOCKET)
		{
			stiOSClose (m_nClientSocket);
			m_nClientSocket = stiINVALID_SOCKET;
		}
	}
	
	return m_nClientSocket;
}
#else
//Android has problems trying to use ifaddrs so we need to use the old method.
////////////////////////////////////////////////////////////////////////////////
//; VrclConnect
//
// Description: Establish a remote VRCL connection.
//
// Returns:
//	A handle to the new connection or 0 upon failure.
//
int CstiVRCLClient::VrclConnect (
	const char *pszRemoteHost,
	uint16_t un16Port)
{
	#define MAXBUF 1024
	stiHResult hResult = stiRESULT_SUCCESS;
	std::string LocalIP;
	int syserr=0;
	unsigned int nLocalIp;
	sockaddr_in servaddr;
	sockaddr_in *pSockAddr_in = NULL;
	addrinfo *pstAddrInfo = NULL;
	CstiHostNameResolver *poResolver = CstiHostNameResolver::getInstance ();

	// Get the local IP address
	hResult = stiGetLocalIp (&LocalIP, estiTYPE_IPV4);
	stiTESTRESULT ();

#ifndef WIN32
	// TODO: this is BAD!!!
	inet_aton (LocalIP.c_str (), (in_addr*)&nLocalIp);
#else
	// TODO: this is BAD!!!
	nLocalIp = inet_addr (LocalIP->c_str ());
#endif
	hResult = poResolver->Resolve (pszRemoteHost, NULL, &pstAddrInfo);
	stiTESTRESULT ();

	// we better get a HostTable from the prior two calls, otherwise we are done
	stiTESTCOND (pstAddrInfo && pstAddrInfo->ai_addr, stiRESULT_ERROR);

	pSockAddr_in = (sockaddr_in *)pstAddrInfo->ai_addr;

	//
	// If the port specified is zero then use the standard VRCL port
	//
	if (un16Port == 0)
	{
		un16Port = VRCL_PORT_DEFAULT;
	}

	// setup the servaddr structure
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr =  pSockAddr_in->sin_addr;
	servaddr.sin_port = htons (un16Port);

	// Start connecting!
	m_nClientSocket = stiOSSocket (servaddr.sin_family, SOCK_STREAM, 0); // TODO: may want SOCK_DGRAM (chunks) instead
	if (m_nClientSocket == -1)
	{
		stiTrace ("ERR: (%d) Unable to allocate socket.\n", m_nClientSocket);
		m_nClientSocket =  stiINVALID_SOCKET;
		stiTHROW (stiRESULT_ERROR);
	}

	hResult = stiConnectWithTimeout (m_nClientSocket, (struct sockaddr*)&servaddr, sizeof (servaddr), nMAX_CONNECT_TIMEOUT);
	if (stiIS_ERROR(hResult))
	{
		syserr = errno;
		stiTrace ("ERR: System error connecting \"%s\" socket (%d) %s\n", pszRemoteHost, syserr, strerror (syserr));
		m_nClientSocket = stiINVALID_SOCKET;
		stiTHROW (stiRESULT_ERROR);
	}
	// We've got a network connection, now to log in...
	char key[MAX_KEY];

	stiKeyCreate (pSockAddr_in->sin_addr.s_addr, nLocalIp, key);
	char szBuf[MAXBUF];
	sprintf (szBuf, "%s%s%s", g_szConnect, key, g_szConnectEnd);

	hResult = VrclWrite (szBuf, NULL);
	stiTESTRESULT ();

	// Get the result
	hResult = VrclRead (szBuf, MAXBUF, NULL);
	stiTESTRESULT ();

	// Split this into 2 strings: the xml tag, and the stuff inside it (if any);
	char *pszInnards;
	pszInnards = szBuf;
	for (; *pszInnards; ++pszInnards)
	{
		if (*pszInnards == '>')
		{
			*pszInnards='\0';
			++pszInnards;
			for (int i = 0; pszInnards[i]; ++i) // break at the next tag
			{
				if (pszInnards[i] == '<')
				{
					pszInnards[i]='\0';
					break;
				}
			}
			break;
		}
	}

	//
	if (0 == strcmp (szBuf, g_szConnectAckMessage))
	{
		// simple authentication (so we're good)
	}
	else if (0 == strcmp (szBuf, g_szConnectAckBeginMessage))
	{
		char szPassword[MAX_PASSWORD];

		stiPasswordCreate (key, pszInnards, szPassword);

		sprintf (szBuf, "%s%s%s", g_szValidate, szPassword, g_szValidateEnd);

		hResult = VrclWrite (szBuf, NULL);
		stiTESTRESULT ();

		// Get the result
		//sleep (1);
		hResult = VrclRead (szBuf, MAXBUF, NULL);
		stiTESTRESULT ();

		// Did it work?
		if(0 != strcmp (szBuf, g_szValidateAckMessage))
		{
			stiTrace ("ERR: Authentication failed.\n\t%s", szBuf);
			stiOSClose (m_nClientSocket);
			m_nClientSocket = stiINVALID_SOCKET;
			stiTHROW (stiRESULT_ERROR);
		}
	}

	// Make sure the client knows what vrcl version we'll be using
	sprintf (szBuf, "%s%s%s", g_szAPIVersionGet, g_szAPIVersion, g_szAPIVersionGetEnd);

	hResult = VrclWrite (szBuf, NULL);
	stiTESTRESULT ();

	hResult = VrclRead (szBuf, MAXBUF, NULL);
	stiTESTRESULT ();

	//
	// <APIVersion>
	//
	if (0 == strncmp (
		szBuf,
		g_szAPIVersionMessage,
		sizeof (g_szAPIVersionMessage) - 1))
	{
		// Grab the API version sent by the server.
		char *pszVersion = szBuf + (sizeof (g_szAPIVersionMessage) - 1);

		// Store the API version sent by the server.
		sscanf (
			pszVersion,
			"%d.%d",
			&m_nServerAPIMajorVersion,
			&m_nServerAPIMinorVersion);
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		if (m_nClientSocket != stiINVALID_SOCKET)
		{
			stiOSClose (m_nClientSocket);
			m_nClientSocket = stiINVALID_SOCKET;
		}
	}

	return m_nClientSocket;
}
#endif


stiHResult CstiVRCLClient::MessageProcess (char *byBuffer, int nBytesReadFromPort)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	char *pszNodeType = nullptr;
	char *pszNodeType2 = nullptr;

	if (nBytesReadFromPort > 0)
	{
		char szXMLTagBuffer[stiMAX_VRCL_READ_SIZE + 1];
		char * pszXMLTagBuffer = szXMLTagBuffer;
		char * pchData;
		bool bHandled = true;

		//
		// NULL terminate the string.
		//
		byBuffer[nBytesReadFromPort] = 0;

		uint32_t un32TagSize = 0;
		uint32_t un32DataSize = 0;
		EstiBool bTagFound = estiTRUE;
		EstiBool bInvalidTagFound = estiFALSE;

		stiDEBUG_TOOL(g_stiVRCLServerDebug,
			stiTrace ("%sCstiVRCLClient::MessageProcess - received %d bytes:"
			"\"%s\"\n", m_strClientID, nBytesReadFromPort, byBuffer);
		);

		//
		// Write the incoming data to the circular buffer
		//
		uint32_t un32NumberOfBytesWritten = 0;

		m_oMessageBuffer.Write (
			byBuffer,
			nBytesReadFromPort,
			un32NumberOfBytesWritten);

		for (;;)
		{
			//
			// Read back tags from the circular buffer until there are no more
			//
			pszXMLTagBuffer = szXMLTagBuffer;

			hResult = m_oMessageBuffer.GetNextXMLTag (
				pszXMLTagBuffer,
				stiMAX_VRCL_READ_SIZE,
				un32TagSize,
				bTagFound,
				&pchData,
				un32DataSize,
				bInvalidTagFound);
			stiTESTRESULT();

			if (!bTagFound)
			{
				stiDEBUG_TOOL(g_stiVRCLServerDebug,
						stiTrace ("%sCstiVRCLClient::MessageProcess - Tag NOT FOUND!\n", m_strClientID);
				);

				break;
			} // end if

			//
			// Skip past any whitespace at the beginning.
			//
			for (; *pszXMLTagBuffer != '\0' && isspace (*pszXMLTagBuffer); ++pszXMLTagBuffer)
			{
			}

			//
			// If we have a doc and related data then destroy it before creating a new one.
			//
			if (m_pxDoc)
			{
				CleanupXmlDocument(m_pxDoc);

				m_pxDoc = nullptr;
				m_pRootNode = nullptr;
			}

			if (pszNodeType)
			{
				delete [] pszNodeType;
				pszNodeType = nullptr;
			}

			if (pszNodeType2)
			{
				delete [] pszNodeType2;
				pszNodeType2 = nullptr;
			}

			//
			// Try creating a new one.
			//
			hResult = CreateXmlDocumentFromString(pszXMLTagBuffer, &m_pxDoc);

			if (!stiIS_ERROR(hResult) && m_pxDoc)
			{
				hResult = GetChildNodeType((IXML_Node *)m_pxDoc,
										   &pszNodeType,
										   &m_pRootNode);
			}

			if (pszNodeType)
			{
				//
				// <SignMailRecordingEnabled />
				//
				if (0 == strncmp (
					pszNodeType,
					g_szSignMailRecordingEnabled,
					strlen (g_szSignMailRecordingEnabled)))
				{
					Callback (estiMSG_VRCL_SIGNMAIL_ENABLED, (size_t)0);
				}

				//
				// <SignMailAvailable />
				//
				else if (0 == strncmp (
					pszNodeType,
					g_szSignMailAvailable,
					strlen (g_szSignMailAvailable)))
				{
					Callback (estiMSG_VRCL_SIGNMAIL_AVAILABLE, (size_t)0);
				}

				//
				// <SignMailGreetingStarted />
				//
				else if (0 == strncmp (
					pszNodeType,
					g_szSignMailGreetingStarted,
					strlen (g_szSignMailGreetingStarted)))
				{
					Callback (estiMSG_VRCL_SIGNMAIL_GREETING_STARTED, (size_t)0);
				}

				else
				{
					bHandled = false;
				}
			}

			if (!bHandled)
			{
				// If the XML is to be printed, do so here.  Trap IsAlive and CallInProgress messages though.
				if (m_bXmlVerbose &&
					0 != strncmp ("<IsAlive", pszXMLTagBuffer, strlen ("<IsAlive")) &&
					0 != strncmp ("<CallInProgress", pszXMLTagBuffer, strlen ("<CallInProgress")))
				{
					stiTrace ("%s\"%s\" \n", m_strClientID, pszXMLTagBuffer);
				}

				//
				// <IsAlive>
				//
				if (0 == strcmp (
					pszXMLTagBuffer,
					g_szIsAliveMessage))
				{
					hResult = VrclWrite (g_szIsAliveAck, nullptr);
					stiTESTRESULT ();
				}

				//
				// <StatusInCallEx>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szStatusInCallExMessage,
					sizeof (g_szStatusInCallExMessage) - 1))
				{
					Callback (estiMSG_CONFERENCING, (size_t)0);
				}

				//
				// <CallConnectedEx>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallConnectedExMessage,
					sizeof (g_szCallConnectedExMessage) - 1))
				{
				}

				//
				// <StatusConnecting>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szStatusConnectingMessage,
					sizeof (g_szStatusConnectingMessage) - 1))
				{
					auto pszData = new char[un32DataSize + 1];
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					// Check to see if the call is incoming
					if (0 == strncmp (
						pszData,
						g_szIncomingCallMessage,
						un32DataSize - 1))
					{
						Callback (estiMSG_VRCL_CLIENT_STATUS_INCOMING, (size_t)0);
					}
					else if (0 == strncmp (
						pszData,
						g_szOutgoingCallMessage,
						un32DataSize - 1))
					{
						Callback (estiMSG_VRCL_CLIENT_STATUS_OUTGOING, (size_t)0);
					}

					delete [] pszData;
				}

				//
				// <CallDialed>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallDialed,
					sizeof (g_szCallDialed) - 1))
				{
					Callback (estiMSG_VRCL_CLIENT_CALL_DIALED, (size_t)0);
				}

				//
				// <CallTerminated>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallTerminated,
					sizeof (g_szCallDialed) - 1))
				{
					Callback (estiMSG_VRCL_CLIENT_CALL_TERMINATED, (size_t)0);
				}

				//
				// <CallInProgress>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallInProgressMessage,
					sizeof (g_szCallInProgressMessage) - 1))
				{
				}

				//
				// <StatusIdle>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szStatusIdleMessage,
					sizeof (g_szStatusIdleMessage) - 1))
				{
					Callback (estiMSG_DISCONNECTED, (size_t)0);
				}

				//
				// <ApplicationVersion>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szApplicationVersionMessage,
					sizeof (g_szApplicationVersionMessage) - 1))
				{
					auto pszData = new char[un32DataSize + 1]; // To be deleted by the test application once it receives the callback.
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					Callback (estiMSG_VRCL_CLIENT_APPLICATION_VERSION, (size_t)pszData);
				}


				//
				// <CallIsTransferable>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallIsTransferableMessage,
					sizeof (g_szCallIsTransferableMessage) - 1))
				{
					Callback (estiMSG_VRCL_CLIENT_CALL_TRANSFERABLE, (size_t)1);
				}

				//
				// <CallTransferSuccess>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallTransferSuccess,
					sizeof (g_szCallTransferSuccess) - 1))
				{
					Callback (estiMSG_VRCL_CLIENT_CALL_TRANSFER_SUCCESS, (size_t)0);
				}

				//
				// <CallIsNotTransferable>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallIsNotTransferableMessage,
					sizeof (g_szCallIsNotTransferableMessage) - 1))
				{
					Callback (estiMSG_VRCL_CLIENT_CALL_TRANSFERABLE, (size_t)0);
				}

				//
				// <CallBandwidthDetectionBeginSuccess>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szCallBandwidthDetectionBeginSuccessMessage,
					sizeof (g_szCallBandwidthDetectionBeginSuccessMessage) - 1))
				{
					Callback (estiMSG_VRCL_CLIENT_CALL_BANDWIDTH_DETECTION_BEGIN_SUCCESS, (size_t)0);
				}

				//
				// <DeviceTypeMessage>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szDeviceTypeMessage,
					sizeof (g_szDeviceTypeMessage) - 1))
				{
					auto pszData = new char[un32DataSize + 1]; // To be deleted by the test application once it receives the callback.
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					Callback (estiMSG_VRCL_DEVICE_TYPE, (size_t)pszData);
				}

				//
				// <AudioPrivacySetSuccess>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szAudioPrivacySetSuccessMessage,
					sizeof (g_szAudioPrivacySetSuccessMessage) - 1))
				{
					Callback (estiMSG_VRCL_AUDIO_PRIVACY_SET_SUCCESS, (size_t)0);
				}
				
				//
				// <AllowedAudioCodecsSuccess>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szAllowedAudioCodecsSuccessMessage,
					sizeof (g_szAllowedAudioCodecsSuccessMessage) - 1))
				{
					Callback (estiMSG_VRCL_ALLOWED_AUDIO_CODECS_SUCCESS, (size_t)0);
				}
				
				//
				// <AllowedAudioCodecsFail>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szAllowedAudioCodecsFailMessage,
					sizeof (g_szAllowedAudioCodecsFailMessage) - 1))
				{
					Callback (estiMSG_VRCL_ALLOWED_AUDIO_CODECS_FAIL, (size_t)0);
				}
				
				//
				// <VideoPrivacySetSuccess>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szVideoPrivacySetSuccessMessage,
					sizeof (g_szVideoPrivacySetSuccessMessage) - 1))
				{
					Callback (estiMSG_VRCL_VIDEO_PRIVACY_SET_SUCCESS, (size_t)0);
				}

				//
				// <LocalNameSetSuccess>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szLocalNameSetSuccessMessage,
					sizeof (g_szLocalNameSetSuccessMessage) - 1))
				{
					Callback (estiMSG_VRCL_LOCAL_NAME_SET_SUCCESS, (size_t)0);
				}

				//
				// <Setting Type>  // Response to SettingGet
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szSettingGetMessage,
					sizeof (g_szSettingGetMessage) - 1))
				{
					auto pszData = new char[un32DataSize + 1]; // To be deleted by the test application once it receives the callback.
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					Callback (estiMSG_VRCL_SETTING_GET_SUCCESS, (size_t)pszData);
				}

				//
				// <g_szSettingSetSuccess>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szSettingSetSuccessMessage,
					sizeof (g_szSettingSetSuccessMessage) - 1))
				{
					Callback (estiMSG_VRCL_SETTING_SET_SUCCESS, (size_t)0);
				}

				//
				// <CheckUpdateResult>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szUpdateCheckResultMessage,
					sizeof (g_szUpdateCheckResultMessage) - 1))
				{
					auto pszData = new char[un32DataSize + 1]; // To be deleted by the test application once it receives the callback.
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					Callback (estiMSG_CHECK_UPDATE_RESULT, (size_t)pszData);
				}

				//
				// <HearingStatusResponse>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szHearingStatusResponseStart,
					sizeof (g_szHearingStatusResponseStart) - 1))
				{
					auto pszData = new char[un32DataSize + 1]; // To be deleted by the test application once it receives the callback.
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					Callback (estiMSG_VRCL_HEARING_STATUS_RESPONSE, (size_t)pszData);
				}

				//
				// <RelayNewCallReadyResponse>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szRelayNewCallReadyResponseStart,
					sizeof (g_szRelayNewCallReadyResponseStart) - 1))
				{
					auto pszData = new char[un32DataSize + 1]; // To be deleted by the test application once it receives the callback.
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					Callback (estiMSG_VRCL_RELAY_NEW_CALL_READY_RESPONSE, (size_t)pszData);
				}

				//
				// <ContactShare>
				//
				else if (0 == strncmp (
					pszXMLTagBuffer,
					g_szContactShareStart,
					sizeof (g_szContactShareStart) - 1))
				{
					auto pszData = new char[un32DataSize + 1]; // To be deleted by the test application once it receives the callback.
					memcpy (pszData, pchData, un32DataSize);
					pszData[un32DataSize] = 0;

					Callback (estiMSG_VRCL_HEARING_CALL_SPAWN, (size_t)pszData);
				}
			} // end while
		}
	} // end if

STI_BAIL:

	if (pszNodeType)
	{
		delete [] pszNodeType;
		pszNodeType = nullptr;
	}

	if (pszNodeType2)
	{
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

	return stiRESULT_SUCCESS;
}

void CstiVRCLClient::RemoveQuotes(std::string &str)
{
	size_t pos;
	while((pos = str.find('"')) != std::string::npos)
	{
	      str.erase(pos,1);
	}
}

stiHResult CstiVRCLClient::textInSignMail (
	const std::string &signMailText,
	const std::string &startSeconds,
	const std::string &endSeconds)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TextInSignMail);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	auto *textInfo = new (SstiTextInSignMailInfo);
	CstiExtendedEvent oEvent (estiEVENT_TEXT_IN_SIGNMAIL, (stiEVENTPARAM)textInfo, 0);

	stiTESTCOND(!signMailText.empty() && !startSeconds.empty() && !endSeconds.empty(), 
				stiRESULT_INVALID_PARAMETER);    

	textInfo->signMailText = signMailText;
	textInfo->startSeconds = startSeconds;
	textInfo->endSeconds = endSeconds;

	hResult = MsgSend (&oEvent, sizeof (oEvent));
		
STI_BAIL:
		
	if (stiIS_ERROR (hResult))
	{
		delete textInfo;
		textInfo = nullptr;
	}
    
	return (hResult);
}

////////////////////////////////////////////////////////////////////////////////
//; textInSignMailHandle
//
// Description: Set the text on the SignMail.
//
// Abstract:
//	
//
// Returns:
//
//
stiHResult CstiVRCLClient::textInSignMailHandle(
	CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::textInSignMailHandle);
	stiHResult hResult = stiRESULT_SUCCESS;
	
	auto *pExtEvent = (CstiExtendedEvent*)poEvent;
	char *requestString = nullptr;
	auto *textInfo = (SstiTextInSignMailInfo*)(pExtEvent->ParamGet (0));
	stiTESTCOND (textInfo, stiRESULT_ERROR);
	
	// create a new xml document
	
	/* Create the XML document to look like:
	 * <TextInSignMail>
	 * 	<SignMailText>TextToBeDisplayed</SignMailText>
	 * 	<StartSeconds>X</StartSeconds>
	 * 	<EndSeconds>X</EndSeconds>
	 * </TextInSignMail>
	 */
	
	hResult = InitXmlDocument(&m_pxDoc, &m_pRootNode, (char *)g_szTextInSignMail);
	stiTESTRESULT ();

	if (!textInfo->signMailText.empty())
	{
		hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
										g_szSignMailText, textInfo->signMailText.c_str ());
		stiTESTRESULT ();

		hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
										g_szStartSeconds, textInfo->startSeconds.c_str ());
		stiTESTRESULT ();
		
		hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
										g_szEndSeconds, textInfo->endSeconds.c_str ());
		stiTESTRESULT ();
	}
	
	hResult = GetXmlString((IXML_Node *)m_pxDoc, &requestString);
	stiTESTRESULT ();
	
	hResult = VrclWrite (requestString, nullptr);
	stiTESTRESULT ();
	
STI_BAIL:
	if (m_pxDoc)
	{
		CleanupXmlDocument(m_pxDoc);
		m_pxDoc = nullptr;
		m_pRootNode = nullptr;
	}
	
	if (requestString)
	{
		delete []requestString;
		requestString = nullptr;
	}
	
	// Free up the memory allocated to send within the event.
	if (textInfo)
	{
		delete textInfo;
		textInfo = nullptr;
	}
	
	return (hResult);	
}

stiHResult CstiVRCLClient::callIdSet (
	const std::string &callId)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::TextInSignMail);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	char *terpCallId = nullptr;
	stiTESTCOND(!callId.empty(), stiRESULT_INVALID_PARAMETER);    

	terpCallId = new char[callId.length() + 1];
	stiTESTCOND(terpCallId, stiRESULT_INVALID_PARAMETER);    

	strcpy (terpCallId, callId.c_str());

	{
		CstiExtendedEvent oEvent (estiEVENT_CALL_ID_SET, (stiEVENTPARAM)terpCallId, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
	}


STI_BAIL:

	if (stiIS_ERROR(hResult) &&
		terpCallId != nullptr)
	{
		delete []terpCallId;
		terpCallId = nullptr;
	}
	return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//; callIdSetHandle
//
// Description: Set the text on the SignMail.
//
// Abstract:
//	
//
// Returns:
//
//
stiHResult CstiVRCLClient::callIdSetHandle(
	CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::callIdSetHandle);
	stiHResult hResult = stiRESULT_SUCCESS;
	std::stringstream xmlBuf;

	auto pExtEvent = (CstiExtendedEvent*)poEvent;
	auto callId = (char*)(pExtEvent->ParamGet (0));

	stiTESTCOND (callId, stiRESULT_ERROR);

	xmlBuf << "<" << g_szCallIdSet << ">" << callId << "</" << g_szCallIdSet << ">";

	hResult = VrclWrite (xmlBuf.str().c_str(), nullptr);
	stiTESTRESULT ();

STI_BAIL:

	// Delete the dial string that was allocated by the call to Dial.
	if (callId)
	{
		delete [] callId;
		callId = nullptr;
	}

	return hResult;
}


stiHResult CstiVRCLClient::hearingVideoCapability (
	const std::string &hearingNumber)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::hearingVideoCapability);
	
	stiHResult hResult = stiRESULT_SUCCESS;
	char *hearingNum = nullptr;
	stiTESTCOND(!hearingNumber.empty(), stiRESULT_INVALID_PARAMETER);    

	hearingNum = new char[hearingNumber.length() + 1];
	strcpy (hearingNum, hearingNumber.c_str());

	{
		CstiExtendedEvent oEvent (estiEVENT_HEARING_VIDEO_CAPABILITY, (stiEVENTPARAM)hearingNum, 0);
		hResult = MsgSend (&oEvent, sizeof (oEvent));
	}

STI_BAIL:

	if (stiIS_ERROR(hResult) &&
		hearingNum != nullptr)
	{
		delete []hearingNum;
		hearingNum = nullptr;
	}

	return hResult;
}


stiHResult CstiVRCLClient::hearingVideoCapabilityHandle (
	CstiEvent *poEvent)
{
	stiLOG_ENTRY_NAME (CstiVRCLClient::hearingVideoCapabilityHandle);
	
	stiHResult hResult = stiRESULT_SUCCESS;

	auto *pExtEvent = (CstiExtendedEvent*)poEvent;
	char *requestString = nullptr;
	auto *hearingNumber = (char *)(pExtEvent->ParamGet (0));
	stiTESTCOND (hearingNumber, stiRESULT_ERROR);
	
	// create a new xml document
	
	/* Create the XML document to look like:
	 * <HearingVideoCapability>
	 * 	<HearingNumber>1234567890</HearingNumber>
	 * </HearingVideoCapability>
	 */
	
	hResult = InitXmlDocument(&m_pxDoc, &m_pRootNode, g_hearingVideoCapability);
	stiTESTRESULT ();

	hResult = AddXmlElementAndValue(m_pxDoc, m_pRootNode, 
									g_hearingNumber, hearingNumber);
	stiTESTRESULT ();
	
	hResult = GetXmlString((IXML_Node *)m_pxDoc, &requestString);
	stiTESTRESULT ();
	
	hResult = VrclWrite (requestString, nullptr);
	stiTESTRESULT ();
	
STI_BAIL:
	if (m_pxDoc)
	{
		CleanupXmlDocument(m_pxDoc);
		m_pxDoc = nullptr;
		m_pRootNode = nullptr;
	}
	
	if (requestString)
	{
		delete []requestString;
		requestString = nullptr;
	}
	
	// Free up the memory allocated to send within the event.
	if (hearingNumber)
	{
		delete hearingNumber;
		hearingNumber = nullptr;
	}

	return hResult;
}
