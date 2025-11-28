/*!
* \file CstiProtocolManager.h
* \brief See CstiProtocolManager.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 - 2017 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiCall.h"
#include "CstiRoutingAddress.h"
#include "IstiVideoOutput.h"


//
// Constants
//
const uint16_t un16STACK_STARTUP_RETRY_WAIT = 20000;  // Time to wait (20 seconds) between attempts to reattempt starting the stack (in milliseconds).



#define stiALWAYS_VALID 0xFFFFFFFF // Mask for call state events that are always valid


//
// Typedefs
//
typedef struct SstiCMInitData
{
	uint8_t un8t35Extension = 0;
	uint8_t un8t35CountryCode = 0;
	uint16_t un16ManufacturerCode = 0;
} SstiCMInitData;


typedef struct SRtpRtcpData
{
	void *pContext = nullptr; /// Used to pass the rtp or rtcp handle
	uint32_t un32RmtIp = 0; /// The remote stations IP address
	uint16_t un16RmtPort = 0; /// The remote stations port
} SRtpRtcpData;


// When creating a call, we will either create a conference or join an
// existing one.  This enumeration is used to select which it will be.
enum EConfType
{
	eCONF_CREATE,
	eCONF_JOIN
};


//
// Forward Declarations
//
class CstiCallStorage;
class IstiBlockListManager2;

//
// Globals
//


//
// Class Declaration
//
class CstiProtocolManager : public CstiEventQueue
{
public:

	 CstiProtocolManager (
		CstiCallStorage *pCallStorage,
		IstiBlockListManager2 *pBlockListManager,
		const std::string &taskName);

	CstiProtocolManager (const CstiProtocolManager &other);
	CstiProtocolManager (CstiProtocolManager &&other);
	CstiProtocolManager &operator= (const CstiProtocolManager &other);
	CstiProtocolManager &operator= (CstiProtocolManager &&other);

	~CstiProtocolManager () override = default;

	// This method is only public to allow it to be called by the callback
	// running in the context of the ConferenceManager's task.  Specifically,
	// the callback that receives incoming events from the pipe via the ITCO.
	virtual stiHResult NextStateSet (
	    CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState = estiSUBSTATE_NONE);

	void UserIdGet (std::string *pUserId) const
		{ *pUserId = m_userId; }

	stiHResult UserIdsSet (
		const std::string &userId,
		const std::string &groupUserId);

	void GroupUserIdGet (std::string *pGroupUserId) const
		{ *pGroupUserId = m_groupUserId ; }

	void DefaultProviderAgreementSet (bool bAgreed);

	unsigned int CallObjectsCountGet (
		uint32_t un32StateMask);

#ifdef stiDEBUGGING_TOOLS
	std::string m_currentEventName; // Used for debugging to print the text string of the current event being processed.
#endif

	uint16_t GetChannelPortBase ();
	uint16_t GetChannelPortRange ();

	stiHResult PlaybackCodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);

	stiHResult RecordCodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);

	void ConfigurationSet (
		EstiInterfaceMode eLocalInterfaceMode,
		bool bEnforceAuthorizedPhoneNumber);

	virtual stiHResult ConferenceParamsSet (
		const SstiConferenceParams *pstConferenceParams);

	void AutoRejectSet (
		bool bAutoReject);

	void MaxCallsSet (
		unsigned int maxCalls);

	void LocalDisplayNameGet (std::string *pDisplayName) const
		{ *pDisplayName = m_displayName; }

	virtual stiHResult LocalDisplayNameSet (
		const std::string &displayName);

	void LocalReturnCallInfoGet (
		EstiDialMethod *eMethod,
		std::string *pDialString)
	    { *eMethod = m_eLocalReturnCallDialMethod; *pDialString = m_localReturnCallDialString; }

	stiHResult LocalReturnCallInfoSet (
		EstiDialMethod eMethod,
	    std::string dialString);

	SstiUserPhoneNumbers *UserPhoneNumbersGet ();

	virtual stiHResult UserPhoneNumbersSet (
		const SstiUserPhoneNumbers *psUserPhoneNumbers);

	virtual stiHResult CallHold (
	    CstiCallSP call) = 0;

	virtual stiHResult CallResume (
	    CstiCallSP call) = 0;

	virtual stiHResult RemoteLightRingFlash (
	    CstiCallSP call) = 0;

	virtual stiHResult CallReject (
	    CstiCallSP call) = 0;

	virtual stiHResult CallAnswer (
	    CstiCallSP call) = 0;

	virtual stiHResult DSNameResolved (
	    CstiCallSP call) = 0;

	virtual stiHResult CallTransfer (
	    CstiCallSP call,
	    std::string dialString) = 0;

	int G711AudioPlaybackMaxSampleRateGet () const;
	int G711AudioRecordMaxSampleRateGet () const;

	void productSet (
		ProductType productType, ///< The product type
		const std::string &version); ///< The version of the product

	ProductType productGet ();

	stiHResult DtmfToneReceived (EstiDTMFDigit eDTMFDigit);

	virtual stiHResult VRSFailoverCallDisconnect (
	    CstiCallSP call) = 0;

public:
	CstiSignal<const SstiStateChange &> callStateChangedSignal;
	CstiSignal<CstiCallSP> callInformationChangedSignal;
	CstiSignal<CstiCallSP> preHangupCallSignal;
	CstiSignal<> pleaseWaitSignal;

protected:
	void eventLoop () override = 0;

	void IncomingRingStart () { stiTrace ("TODO: Remove CstiProtocolManager::IncomingRingStart()\n"); }

	void IncomingRingStop () { stiTrace ("TODO: Remove CstiProtocolManager::IncomingRingStop()\n"); }

	stiHResult OutgoingToneSet (
		EstiToneMode eTone);

	stiHResult CallAccept (
	    CstiCallSP call);
	stiHResult VideoRecordCodecsGet (
		std::list<EstiVideoCodec> *pCodecs);
	stiHResult VideoPlaybackCodecsGet (
		std::list<EstiVideoCodec> *pCodecs);
	void LimitVideoCodecsPerPropertyTable (
		std::list<EstiVideoCodec> *pCodecs);

	stiHResult AudioCodecsGet (
		std::list<EstiAudioCodec> *pCodecs);

	stiHResult VideoPlaybackPacketizationSchemesGet (
		std::list<EstiPacketizationScheme> *pPacketizationSchemes);

	stiHResult VideoRecordPacketizationSchemesGet (
		std::list<EstiPacketizationScheme> *pPacketizationSchemes);


	virtual stiHResult StateConnectedProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

	virtual stiHResult StateConnectingProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

	virtual stiHResult StateCriticalErrorProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

	virtual stiHResult StateDisconnectingProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

	virtual stiHResult StateDisconnectedProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

	virtual stiHResult      StateHoldLclProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

	virtual stiHResult StateHoldRmtProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

	virtual stiHResult StateHoldBothProcess (
		CstiCallSP call,
		EsmiCallState eNewState,
		uint32_t un32NewSubState) = 0;

#ifdef stiDEBUGGING_TOOLS
	// Private Debugging Functions
	virtual void StateChangeTrack (
	    CstiCallSP call,
		EsmiCallState eNewState,
		int32_t n32SubState);
#endif


protected:

	ProductType m_productType{ProductType::Unknown};
	std::string m_version;
	std::string productNameGet ();

	bool m_bDefaultProviderAgreementSigned = false;			// Has the agreement been signed? (Need to pass status to terp.)

	bool m_bRestartStackPending = false;
	bool m_networkChangePending = false;

	IstiBlockListManager2 *m_pBlockListManager = nullptr;
	
	// port settings
	uint16_t m_nChannelPortBase = 49154;
	uint16_t m_nChannelPortRange = 12;
	uint16_t m_nControlPortBase = 49154;
	uint16_t m_nControlPortRange = 12;
	EstiBool  m_bUseSecondPort = estiFALSE;
	uint16_t m_nSecondPort = 0;

	std::string m_userId;
	std::string m_groupUserId;
	std::string m_displayName;
	SstiUserPhoneNumbers m_UserPhoneNumbers = {};	// All the phone numbers associated with the current user
	CstiCallStorage *m_pCallStorage = nullptr;

	EstiInterfaceMode m_eLocalInterfaceMode = estiSTANDARD_MODE;

	unsigned int m_maxCalls {2};

	bool m_bEnforceAuthorizedPhoneNumber = false;

	bool m_bAutoReject = false;

	SstiConferenceParams m_stConferenceParams = {};

	EstiDialMethod	m_eLocalReturnCallDialMethod = estiDM_UNKNOWN;

	std::string m_localReturnCallDialString;

private:

	CstiSignalConnection::Collection m_signalConnections;
};
