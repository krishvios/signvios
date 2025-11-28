/*!
* \file DhvEngine.cpp
* \brief Engine for Wavello that starts up Conference Manager
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2009 - 2019 Sorenson Communications, Inc. -- All rights reserved
*/
#include "DhvEngine.h"
#include "IstiPlatform.h"
#include "stiSSLTools.h"
#include "stiRemoteLogEvent.h"
#include "stiTools.h"
#include "CstiSipCallLeg.h"
#include "IstiNetwork.h"

/*!
// \brief Constructor
//
// \paramapp callback function pointer for callback events
// \param callbackParam object returned to function pointer
*/

DhvEngine::DhvEngine()
{
	conferenceManager = new CstiConferenceManager();

	m_signalConnections.push_back (conferenceManager->incomingCallSignal.Connect (
		[this](CstiCallSP call)
	{
		incomingCallSignal.Emit (call);
	}));

	m_signalConnections.push_back (conferenceManager->outgoingCallSignal.Connect (
		[this](CstiCallSP call)
	{
		outgoingCallSignal.Emit (call);
	}));
	
	m_signalConnections.push_back (conferenceManager->establishingConferenceSignal.Connect (
		[this](CstiCallSP call)
	{
		callEstablishingConferenceSignal.Emit (call);
	}));
	
	m_signalConnections.push_back (conferenceManager->conferencingSignal.Connect (
		[this](CstiCallSP call)
	{
		callConferencingSignal.Emit (call);
	}));

	m_signalConnections.push_back (conferenceManager->disconnectedSignal.Connect (
		[this](CstiCallSP call)
	{
		disconnectedCallSignal.Emit (call);
	}));

	m_signalConnections.push_back (conferenceManager->logCallSignal.Connect (
		[this](CstiCallSP call, CstiSipCallLegSP callLeg)
	{
		if (!callLeg->callLoggedGet())
		{
			callLeg->callLoggedSet(true);
			CallStatusSend(call, callLeg);
		}
	}));
}

/*!
// \brief Destructor
*/
DhvEngine::~DhvEngine()
{
	httpLoggerTask->Shutdown ();
	httpLoggerTask->WaitForShutdown ();

	if (conferenceManager)
	{
		delete conferenceManager;
	}
	if (remoteLoggerService)
	{
		stiRemoteLogEventRemoteLoggingSet(nullptr, nullptr);

		delete remoteLoggerService;
		remoteLoggerService = nullptr;
	}
	if (httpLoggerTask)
	{
		delete httpLoggerTask;
		httpLoggerTask = nullptr;
	}
}


/*!
// \brief Initializer for DhvEngine
//
// \param productName the name of the product
// \param version the version of the product
// \param conferenceParams the configuration of the conference manager
// \param conferencePorts the ports used by the conference manager
//
// \return stiRESULT_SUCCESS
*/
stiHResult DhvEngine::initialize(ProductType productType,
	const char *version,
	const char *deviceModel,
	const char *osVersion,
	const std::string& sessionId,
	const SstiConferenceParams *conferenceParams,
	const SstiConferencePorts *conferencePorts)
{
	if (conferenceManager)
	{
		auto platform = IstiPlatform::InstanceGet ();

		platform->Initialize ();

		stiHResult hResult = conferenceManager->Initialize(
			0,  // SCI-ALA Disable blocklistManager - the Hold Server does not need this
			productType,
			version,
			conferenceParams,
			conferencePorts);
		hResult = conferenceManager->MaxCallsSet(MaxCalls);
		hResult = conferenceManager->AutoRejectSet(estiOFF);
		conferenceManager->LocalInterfaceModeSet(estiSTANDARD_MODE);
		hResult = conferenceManager->Startup();

		this->deviceModel = deviceModel;
		this->osVersion = osVersion;

		// Initialize the watchdog timers.
		stiOSWdInitialize ();
		CstiWatchDog::Instance().Start();
		
		// Start the shared event queue
		CstiEventQueue::sharedEventQueueGet ().StartEventLoop ();
		
		// Initialize the ErrorLogging system
		stiErrorLogSystemInit (version);

		// Initialize the os layer
		stiOSInit ();

		initializeHTTP ();
		httpLoggerTask->Startup ();

		hResult = initializeLoggingService (productType, version);
		remoteLoggerService->Startup(false);
		remoteLoggerService->SessionIdSet(sessionId);

		return hResult;
	}


	return stiMAKE_ERROR(stiRESULT_ERROR);
}

/*!
// \brief Removes a call object, should be called when the UI is finished with the call object
//
// \param call: the call object to remove
//
// \return stiRESULT_SUCCESS
*/
stiHResult DhvEngine::callObjectRemove(IstiCall * call)
{
	if (call && conferenceManager)
	{
		return conferenceManager->CallObjectRemove(static_cast<CstiCall*>(call));
	}
	return stiMAKE_ERROR(stiRESULT_ERROR);
}

/*!
// \brief Returns the number of call objects in the conference manager.
//
// \return Number of Call Objects
*/
int DhvEngine::callObjectsCountGet()
{
	if (conferenceManager)
	{
		conferenceManager->CallObjectsCountGet();
	}
	return 0;
}

/*!
// \brief Shutsdown the conference manager waiting for it to finish shutting down.
//
// \return stiRESULT_SUCCESS
*/
stiHResult DhvEngine::shutdown()
{
	if (conferenceManager)
	{
		stiHResult hResult = conferenceManager->Shutdown();
		return hResult;
	}
	return stiMAKE_ERROR(stiRESULT_ERROR);
}

stiHResult DhvEngine::reinvite (IstiCall * call)
{
	call->Hold ();
	call->Resume ();
	return stiRESULT_SUCCESS;
}

/*!
// \brief Notify the engine that the network has changed so it can reconfigure its connections.
*/
void DhvEngine::networkChangedNotify()
{
	conferenceManager->NetworkChangedNotify();
#ifdef stiTUNNEL_MANAGER
	conferenceManager->TunnelManagerGet()->NetworkChangeNotify();
#endif
}

void DhvEngine::remoteLogDeviceTokenSet (const std::string& deviceToken)
{
	remoteLoggerService->DeviceTokenSet (deviceToken);
}

void DhvEngine::remoteLogNetworkTypeSet (const std::string& networkType)
{
	remoteLoggerService->NetworkTypeSet (networkType);
}

void DhvEngine::remoteLogPhoneNumberSet (const std::string& email)
{
	remoteLoggerService->LoggedInPhoneNumberSet(email.c_str());
}

void DhvEngine::remoteLogEventSend (const std::string& msg)
{
    stiRemoteLogEventSend (msg.c_str ());
}
/*!
// \brief Creates an IDhvEngine
//
// \param appCallback function pointer for callback events
// \param callbackParam object returned to function pointer
//
// \return stiRESULT_SUCCESS
*/
IDhvEngine * CreateDhvEngine()
{
	return new DhvEngine();
}

/*!
// \brief deletes the media server engine
//
*/
void DestroyDhvEngine(IDhvEngine *pEngine)
{
	delete pEngine;
}


void DhvEngine::callDial (const std::string& URI,
		const std::string& hearingPhoneNumber,
		const OptString& localNameOverride,
		const std::string& remoteName,
		const std::string& relayLanguage,
		const std::string& uniqueId,
		std::shared_ptr<IstiCall>& callOutput,
		const std::vector<SstiSipHeader>& additionalHeaders)
{
	CstiRoutingAddress routingAddress (URI.c_str ());
	//Remote Logger needs to know the hearing phone number that was used for this call.
	strcpy (userPhoneNumbers.szLocalPhoneNumber, hearingPhoneNumber.c_str ());
	strcpy (userPhoneNumbers.szPreferredPhoneNumber, hearingPhoneNumber.c_str ());
	strcpy (userPhoneNumbers.szSorensonPhoneNumber, hearingPhoneNumber.c_str ());
	conferenceManager->UserPhoneNumbersSet (&userPhoneNumbers);
	auto call = conferenceManager->CallDial (
		EstiDialMethod::estiDM_BY_DIAL_STRING, 
		routingAddress, 
		URI, 
		localNameOverride,
		remoteName, 
		relayLanguage,
		0,
		additionalHeaders);
	call->subscriptionIdSet (uniqueId);
	callOutput = call;
}

stiHResult DhvEngine::initializeLoggingService(	ProductType productType,
												const std::string &version)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	std::string ServiceContacts[2]{stiREMOTELOGGER_SERVICE_DEFAULT1, stiREMOTELOGGER_SERVICE_DEFAULT2};

	remoteLoggerService = new CstiRemoteLoggerService (httpLoggerTask,
														  nullptr,
														  (size_t)this);

	hResult = remoteLoggerService->Initialize (ServiceContacts,
												  "",
												  version,
												  productType,
												  0,
												  1,
												  1);

	stiAssertRemoteLoggingSet((size_t*)remoteLoggerService, CstiRemoteLoggerService::RemoteAssertSend);
	stiErrorLogRemoteLoggingSet((size_t *)remoteLoggerService, CstiRemoteLoggerService::
								RemoteAssertSend);
	stiRemoteLogEventRemoteLoggingSet((size_t *)remoteLoggerService, CstiRemoteLoggerService::RemoteLogEventSend);

STI_BAIL:
	return hResult;
}

stiHResult DhvEngine::initializeHTTP ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	httpLoggerTask = new CstiHTTPTask (nullptr, (size_t)this);
	httpLoggerTask->Initialize();
STI_BAIL:

	return hResult;
}

/*!\brief Collects and sends call statistics to the remote logger service
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
 * Other status codes that are not implemented yet are defined in a separate document.
 *
 * \param call - The call to collect the stats from.
 */
void DhvEngine::CallStatusSend(CstiCallSP call, CstiSipCallLegSP callLeg)
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
		CallStatus << "\tVrsCallID = " << call->vrsCallIdGet() << std::endl;
		
		CallStatus << "\tUserEndedCall = " << UserEndedCall << std::endl;
		CallStatus << "\tStoreMode = " << StoreMode << std::endl;
		CallStatus << "\tSubscriptionID = \"" << call->subscriptionIdGet() << "\"\n";

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

		//
		// Get the dial string.  If the call is outbound
		// use the original dial string.  If the call is
		// inbound then get the remote dial string.
		//
		if (call->DirectionGet () == estiOUTGOING)
		{
			call->OriginalDialStringGet (&DialString);
		}
		else
		{
			call->RemoteDialStringGet (&DialString);
		}

		CallStatus << "\tDialString = " << DialString << "\n";

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

		if (callLeg != nullptr)
		{
			std::string RemoteProductName;
			callLeg->RemoteProductNameGet (&RemoteProductName);
			CallStatus << "\tRemoteProductName = \"" << RemoteProductName << "\"\n";

			std::string RemoteProductVersion;
			callLeg->RemoteProductVersionGet (&RemoteProductVersion);
			CallStatus << "\tRemoteProductVersion = " << RemoteProductVersion << "\n";
		}

		// Device Model
		CallStatus << "\tDeviceModel = \"" << deviceModel << "\"\n";

		// System Version
		CallStatus << "\tSystemVersion = " << osVersion << "\n";

		// Network Type
		CallStatus << "\tNetworkType = " << this->networkType << std::endl;

		// Public IP
		SstiConferenceParams stConferenceParams;
		stiHResult hResult = conferenceManager->ConferenceParamsGet (&stConferenceParams);
		if (!stiIS_ERROR (hResult) && !stConferenceParams.prioritizePrivacy)
		{
			CallStatus << "\tPublicIP = " << stConferenceParams.PublicIPv4 << "\n";
			CallStatus << "\tNatTraversalSIP = " << stConferenceParams.eNatTraversalSIP << "\n";
		}

		EstiProtocol eProtocol = call->ProtocolGet ();

		CallStatus << "\tProtocol = " << (estiSIP == eProtocol ? "SIP" : "Unknown") << "\n";

		if (call->unregisteredCallGet ())
		{
			CallStatus << "\tUnregisteredCall = 1\n";
		}

		//
		// Encryption reporting
		//
		CallStatus << "\tSecureCallMode = " << stConferenceParams.eSecureCallMode << "\n";
		
		//
		// Platform specific information
		//
		IstiPlatform::InstanceGet ()->callStatusAdd (CallStatus);

		if (callLeg && callLeg->ConferencedGet ()) {
			// Add preferred display size stats to call stats.
			std::string displaySizeStats;
			call->PreferredDisplaySizeStatsGet(&displaySizeStats);
			CallStatus << displaySizeStats;
			//
			// Received Video Stats (playback)
			//
			CallStatus << "\nReceived Video Stats:\n";
			CallStatus << "\tVideoPacketQueueEmptyErrors = "
					   << stCallStatistics.videoPacketQueueEmptyErrors << "\n";
			CallStatus << "\tVideoPacketsRead = " << stCallStatistics.videoPacketsRead << "\n";
			CallStatus << "\tVideoKeepAlivePackets = " << stCallStatistics.videoKeepAlivePackets
					   << "\n";
			CallStatus << "\tVideoPayloadTypeErrors = "
					   << stCallStatistics.videoUnknownPayloadTypeErrors << "\n";
			CallStatus << "\tVideoPacketsDiscardedReadMuted = "
					   << stCallStatistics.videoPacketsDiscardedReadMuted << "\n";
			CallStatus << "\tVideoPacketsDiscardedPlaybackMuted = "
					   << stCallStatistics.videoPacketsDiscardedPlaybackMuted << "\n";
			CallStatus << "\tVideoPacketsDiscardedEmpty = "
					   << stCallStatistics.videoPacketsDiscardedEmpty << "\n";
			CallStatus << "\tVideoPacketsUsingPreviousSSRC = "
					   << stCallStatistics.videoPacketsUsingPreviousSSRC << "\n";
			CallStatus << "\tTotalPacketsReceived = " << stCallStatistics.Playback.totalPacketsReceived
					   << "\n";
			CallStatus << "\tReceivedPacketLoss = " << stCallStatistics.fReceivedPacketsLostPercent
					   << "\n";
			CallStatus << "\tActualVideoKbpsRecv = "
					   << stCallStatistics.Playback.nActualVideoDataRate << "\n";
			CallStatus << "\tActualFPSRecv = " << stCallStatistics.Playback.nActualFrameRate
					   << "\n";
			CallStatus << "\tVideoFrameSizeRecv = \"" << stCallStatistics.Playback.nVideoWidth
					   << " x " << stCallStatistics.Playback.nVideoHeight << "\"\n";
			CallStatus << "\tVideoCodecRecv = "
					   << VideoCodecToString(stCallStatistics.Playback.eVideoCodec) << "\n";
			CallStatus << "\tVideoTimeMutedByRemote = "
					   << stCallStatistics.videoTimeMutedByRemote.count() / 1000.0 << "\n";
			CallStatus << "\tTotalFramesAssembledAndSentToDecoder = "
					   << stCallStatistics.un32FrameCount << "\n";

			CallStatus << "\tVideoFramesSentToPlatform = "
					   << stCallStatistics.videoFramesSentToPlatform << "\n";
			CallStatus << "\tVideoPartialKeyFramesSentToPlatform = "
					   << stCallStatistics.videoPartialKeyFramesSentToPlatform << "\n";
			CallStatus << "\tVideoPartialNonKeyFramesSentToPlatform = "
					   << stCallStatistics.videoPartialNonKeyFramesSentToPlatform << "\n";
			CallStatus << "\tVideoWholeKeyFramesSentToPlatform = "
					   << stCallStatistics.videoWholeKeyFramesSentToPlatform << "\n";
			CallStatus << "\tVideoWholeNonKeyFramesSentToPlatform = "
					   << stCallStatistics.videoWholeNonKeyFramesSentToPlatform << "\n";

			CallStatus << "\tLostPackets = " << stCallStatistics.Playback.totalPacketsLost << "\n";
			CallStatus << "\tHighestPacketLossSeen = " << stCallStatistics.highestPacketLossSeen
					   << "\n";
			CallStatus << "\tNumberOfTimesPacketLossReported = "
					   << stCallStatistics.countOfCallsToPacketLossCountAdd << "\n";
			CallStatus << "\tDiscardedFromBurst = " << stCallStatistics.burstPacketsDropped << "\n";
			CallStatus << "\tVideoPlaybackFrameGetErrors = "
					   << stCallStatistics.un32VideoPlaybackFrameGetErrors << "\n";
			CallStatus << "\tFirNegotiated = " << call->FirNegotiatedGet() << "\n";
			CallStatus << "\tPliNegotiated = " << call->PliNegotiatedGet() << "\n";
			CallStatus << "\tKeyframesRequested = " << stCallStatistics.un32KeyframesRequestedVP
					   << "\n";
			CallStatus << "\tKeyframesReceived = " << stCallStatistics.un32KeyframesReceived
					   << "\n";
			CallStatus << "\tKeyframeWaitTimeMax = " << stCallStatistics.un32KeyframeMaxWaitVP
					   << "\n";
			CallStatus << "\tKeyframeWaitTimeMin = " << stCallStatistics.un32KeyframeMinWaitVP
					   << "\n";
			CallStatus << "\tKeyframeWaitTimeAvg = " << stCallStatistics.un32KeyframeAvgWaitVP
					   << "\n";
			CallStatus << "\tKeyframeWaitTimeTotal = " << stCallStatistics.fKeyframeTotalWaitVP
					   << "\n";
			CallStatus << "\tErrorConcealmentEvents = "
					   << stCallStatistics.un32ErrorConcealmentEvents << "\n";
			CallStatus << "\tI-FramesDiscardedAsIncomplete = "
					   << stCallStatistics.un32IFramesDiscardedIncomplete << "\n";
			CallStatus << "\tP-FramesDiscardedAsIncomplete = "
					   << stCallStatistics.un32PFramesDiscardedIncomplete << "\n";
			CallStatus << "\tGapsInFrameNumbers = " << stCallStatistics.un32GapsInFrameNumber
					   << "\n";

			//
			// NACK and RTX Stats
			//
			CallStatus << "\nNACK RTX Stats:\n";
			CallStatus << "\tNackRtxVideoNegotiated = " << call->NackRtxNegotiatedGet() << "\n";
			CallStatus << "\tActualVPPacketsLost = " << stCallStatistics.actualVPPacketLoss << "\n";
			CallStatus << "\tNackVideoRequestsSent = " << stCallStatistics.Playback.totalNackRequestsSent << "\n";
			CallStatus << "\tNackVideoRequestsReceived = " << stCallStatistics.Record.totalNackRequestsReceived
					   << "\n";
			CallStatus << "\tRtxVideoRequestsIgnored = " << stCallStatistics.rtxPacketsIgnored
					   << "\n";
			CallStatus << "\tRtxVideoPacketsReceived = " << stCallStatistics.Playback.totalRtxPacketsReceived
					   << "\n";
			CallStatus << "\tRtxVideoPacketsSent = " << stCallStatistics.Record.totalRtxPacketsSent << "\n";
			CallStatus << "\tRtxVideoPacketsNotFound = " << stCallStatistics.rtxPacketsNotFound
					   << "\n";
			CallStatus << "\tVideoPlaybackDelay = " << stCallStatistics.avgPlaybackDelay << "\n";
			CallStatus << "\tActualRtxVideoKbpsSent = " << stCallStatistics.rtxKbpsSent << "\n";
			CallStatus << "\tNumberOfDuplicatePackets = "
					   << stCallStatistics.duplicatePacketsReceived << "\n";
			CallStatus << "\tNackDueToDelay = " << stCallStatistics.rtxFromNoData << "\n";
			CallStatus << "\tAvgRTT = " << stCallStatistics.avgRtcpRTT << "\n";
			CallStatus << "\tAvgJitter = " << stCallStatistics.avgRtcpJitter << "\n";

			if (stCallStatistics.actualVPPacketLoss > highPacketCount ||
				stCallStatistics.burstPacketsDropped > highPacketCount) {
				CallStatus << "\tPossibleStatsError = 1\n";
			}

			//
			// Sent Video Stats (record)
			//
			CallStatus << "\nSent Video Stats:\n";
			CallStatus << "\tVideoPacketsSent = " << stCallStatistics.videoPacketsSent << "\n";
			CallStatus << "\tVideoPacketSendErrors = " << stCallStatistics.videoPacketSendErrors
					   << "\n";
			CallStatus << "\tTargetVideoKbps = " << stCallStatistics.Record.nTargetVideoDataRate
					   << "\n";
			CallStatus << "\tActualVideoKbpsSent = " << stCallStatistics.Record.nActualVideoDataRate
					   << "\n";
			CallStatus << "\tTargetFPS = " << stCallStatistics.Record.nTargetFrameRate << "\n";
			CallStatus << "\tActualFPSSent = " << stCallStatistics.Record.nActualFrameRate << "\n";
			CallStatus << "\tVideoFrameSizeSent = \"" << stCallStatistics.Record.nVideoWidth
					   << " x " << stCallStatistics.Record.nVideoHeight << "\"\n";
			CallStatus << "\tVideoCodecSent = "
					   << VideoCodecToString(stCallStatistics.Record.eVideoCodec) << "\n";
			CallStatus << "\tVRKeyframesRequested = " << stCallStatistics.un32KeyframesRequestedVR
					   << "\n";
			CallStatus << "\tVRKeyframeWaitTimeMax = " << stCallStatistics.un32KeyframeMaxWaitVR
					   << "\n";
			CallStatus << "\tVRKeyframeWaitTimeMin = " << stCallStatistics.un32KeyframeMinWaitVR
					   << "\n";
			CallStatus << "\tVRKeyframeWaitTimeAvg = " << stCallStatistics.un32KeyframeAvgWaitVR
					   << "\n";
			CallStatus << "\tKeyframesSent = " << stCallStatistics.un32KeyframesSent << "\n";
			CallStatus << "\tFramesLostFromRCU = " << stCallStatistics.un32FramesLostFromRcu
					   << "\n";
			CallStatus << "\tPartialFramesSent = " << stCallStatistics.countOfPartialFramesSent
					   << "\n";
			CallStatus << "\tEncryption = " << static_cast<int> (callLeg->encryptionStateVideoGet ()) << "\n";
			CallStatus << "\tKeyExchangeMethod = " << static_cast<int> (callLeg->keyExchangeMethodVideoGet ()) << "\n";
			CallStatus << "\tEncryptionChanges = " << callLeg->encryptionChangesVideoToJsonArrayGet () << "\n";

			// Public IP
			SstiConferenceParams stConferenceParams;
			stiHResult hResult = conferenceManager->ConferenceParamsGet(&stConferenceParams);
			if (!stiIS_ERROR (hResult) && !stConferenceParams.prioritizePrivacy) {
				CallStatus << "\tPublicIP = " << stConferenceParams.PublicIPv4 << "\n";
				CallStatus << "\tNatTraversalSIP = " << stConferenceParams.eNatTraversalSIP << "\n";
			}

			//
			// Flow Control Stats
			//
			CallStatus << "\nFlow Control Information:\n";
			if (!stiIS_ERROR(hResult)) {
				CallStatus << "\tAutoSpeedMode = " << stConferenceParams.eAutoSpeedSetting << "\n";
			}
			CallStatus << "\tMinimumTargetSendSpeed = " << stCallStatistics.un32MinSendRate << "\n";
			CallStatus << "\tMaximumTargetSendSpeedWithAcceptableLoss = "
					   << stCallStatistics.un32MaxSendRateWithAcceptableLoss << "\n";
			CallStatus << "\tMaximumTargetSendSpeed = " << stCallStatistics.un32MaxSendRate << "\n";
			CallStatus << "\tAverageTargetSendSpeedKbps = " << stCallStatistics.un32AveSendRate
					   << "\n";
			CallStatus << "\tTmmbrNegotiated = " << call->TmmbrNegotiatedGet() << "\n";

			//
			// Log the media routing information
			//
			CallStatus << "\nMedia Routing Information:\n";

			CallStatus << "\tRemoteVideoAddr = " << call->RemoteVideoAddrGet() << "\n";
			CallStatus << "\tVideoRemotePort = " << call->VideoRemotePortGet() << "\n";
			if (!stConferenceParams.prioritizePrivacy) {
				CallStatus << "\tLocalVideoAddr = " << call->LocalVideoAddrGet() << "\n";
			}
			CallStatus << "\tVideoPlaybackPort = " << call->VideoPlaybackPortGet() << "\n";

			//
			// Log the ICE results
			//
			CallStatus << "\tICENominationsResult = "
					   << ICENominationsResultStringGet(callLeg->iceNominationResultGet()) << "\n";

			CallStatus << "\tRemoteIceSupport = "
					   << RemoteIceSupportStringGet(callLeg->m_remoteIceSupport) << "\n";

			//
			// Log the media tranport types used during the call
			//
			EstiMediaTransportType eRtpTransportAudio;
			EstiMediaTransportType eRtcpTransportAudio;
			EstiMediaTransportType eRtpTransportText;
			EstiMediaTransportType eRtcpTransportText;
			EstiMediaTransportType eRtpTransportVideo;
			EstiMediaTransportType eRtcpTransportVideo;

			call->MediaTransportTypesGet(
					&eRtpTransportAudio, &eRtcpTransportAudio,
					&eRtpTransportText, &eRtcpTransportText,
					&eRtpTransportVideo, &eRtcpTransportVideo);

			CallStatus << "\tRtpTransportVideo = " << MediaTransportStringGet(eRtpTransportVideo)
					   << "\n";
			CallStatus << "\tRtcpTransportVideo = " << MediaTransportStringGet(eRtcpTransportVideo)
					   << "\n";
		}
	}

	remoteLoggerService->RemoteCallStatsSend(CallStatus.str().c_str ());
}

/*!\brief Translates an ICE result into a human readable string.
 *
 * \param eICENominationsResult The ICE result to translate.
 */
std::string DhvEngine::ICENominationsResultStringGet (
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
std::string DhvEngine::RemoteIceSupportStringGet (
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
	}

	return "Unknown";
}

/*!\brief Translates a media transport type into a human readable string.
 *
 * \param eTransportType The transport type to translate.
 */
std::string DhvEngine::MediaTransportStringGet (
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


std::string DhvEngine::PublicIPAddressGet() {
    std::stringstream CallStatus;
    // Public IP
    SstiConferenceParams stConferenceParams;
    stiHResult hResult = conferenceManager->ConferenceParamsGet (&stConferenceParams);
    if (!stiIS_ERROR (hResult))
    {
        CallStatus << "\tPublicIP = " << stConferenceParams.PublicIPv4 << "\n";
    }
    return CallStatus.str();
}

void DhvEngine::enableRemoteCallStatsLogging(int remoteCallStatsLoggingValue) {
    remoteLoggerService->RemoteCallStatsLoggingEnable(remoteCallStatsLoggingValue);
}

void DhvEngine::updateSipDomain(std::string sipDomain) {
	SstiConferenceParams conferenceParams = {};
	conferenceManager->ConferenceParamsGet(&conferenceParams);
	conferenceParams.SIPRegistrationInfo.PublicDomain = sipDomain;
	conferenceManager->ConferenceParamsSet(&conferenceParams);
}
