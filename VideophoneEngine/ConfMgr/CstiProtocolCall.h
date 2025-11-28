////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential - Do not distribute
// Copyright 2000-2011 by Sorenson Communications, Inc. All rights reserved.
//
//  Class Name: CstiProtocolCall
//
//  File Name:  CstiProtocolCall.h
//
//  Abstract:
//	The common interface which all protocol call objects must abide by.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIPROTOCOLCALL_H
#define CSTIPROTOCOLCALL_H

#include "stiSVX.h"
//#include "rtp.h"
#include "stiProductCompatibility.h"
#include "CstiCallInfo.h"
#include "stiPayloadTypes.h"
#include "stiConfDefs.h"
#include "IstiCall.h"
#include "IstiVideoInput.h"
#include "IstiVideoOutput.h"
#include "CstiSignal.h"
#include <memory>
#include <mutex>



//
// Constants
//
const int nSTATS_ELEMENTS = 10; // The number of stat elements kept for averaging
const uint8_t un8stiMAC_ADDR_LENGTH = 17;		// The # of chars in a MAC address
const uint8_t un8stiVCO_CALLBACK_LENGTH = 25;	// the # of chars in a VCO callback number

//
// Forward Declarations
//
class CstiCall;
class CstiProtocolManager;
class CstiVideoPlayback;
class CstiVideoRecord;
class CstiAudioPlayback;
class CstiAudioRecord;
class CstiTextPlayback;
class CstiTextRecord;
class IstiBlockListManager2;


class CstiProtocolCall
{
public:
	virtual ~CstiProtocolCall () = default; // This is public and virtual so it can be deleted without knowing protocol

	CstiProtocolCall (const CstiProtocolCall &other) = delete;
	CstiProtocolCall (CstiProtocolCall &&other) = delete;
	CstiProtocolCall &operator= (const CstiProtocolCall &other) = delete;
	CstiProtocolCall &operator= (CstiProtocolCall &&other) = delete;

	CstiCallSP CstiCallGet () const { return m_poCstiCall.lock(); }
	void CstiCallSet (CstiCallSP call) { m_poCstiCall = call; }

	stiHResult Lock () const;
	void Unlock () const;

	void DtmfToneSend (
		EstiDTMFDigit eDTMFDigit);

	// Pure virtual methods that individual protocols MUST implement
	virtual stiHResult Accept () = 0;
	virtual stiHResult HangUp () = 0;
	virtual EstiProtocol ProtocolGet () = 0;
	virtual bool ProtocolModifiedGet () {return false;};
	virtual stiHResult SignMailSkipToRecord() = 0;
	virtual stiHResult TransferToAddress (const std::string &destAddress) = 0;

	virtual stiHResult TextSend (const uint16_t *pun16Text, EstiSharedTextSource sharedTextSource) = 0;

	// Public getters and setters
	inline EstiBool AllowHangUpGet () const
		{
		  Lock ();
		  EstiBool bRet = m_bAllowHangUp;
		  Unlock ();
		  return (bRet);
		};
	inline void AllowHangupSet (EstiBool bAllowHangUp)
		{
		  Lock ();
		  m_bAllowHangUp = bAllowHangUp;
		  Unlock ();
		};

	//
	// For statistics
	//
	void StatisticsGet (SstiCallStatistics *pStats);
	void StatsCollect ();
	void StatsClear ();

	inline EstiBool IsHoldableGet () const
	{
		Lock ();
		EstiBool bRet = m_bIsHoldable;
		if (RemoteDeviceTypeIs(estiDEVICE_TYPE_SVRS_HOLD_SERVER))
		{
			bRet = estiFALSE;
		}
		Unlock ();
		return (bRet);
	};
	
	inline void IsHoldableSet (EstiBool bIsHoldable = estiTRUE)
		{
		  Lock ();
		  m_bIsHoldable = bIsHoldable;
		  Unlock ();
		};
	inline int32_t RateGet ()
		{
			Lock ();
			int32_t ret = m_callRate;
			Unlock ();
			return ret;
		};
	virtual int RemotePtzCapsGet () const = 0;
	void VideoPlaybackSizeGet (uint32_t *pun32Width, uint32_t *pun32Height) const;

	inline int ActualPlaybackTotalDataRateGet ()
		{
		  Lock ();
		  int nRet = m_stCallStatistics.Playback.nActualVideoDataRate +
				m_stCallStatistics.Playback.nActualAudioDataRate + 
				m_stCallStatistics.Playback.nActualTextDataRate;
		  Unlock ();
		  return (nRet);
		};

	void RemoteDisplayNameGet (std::string *pRemoteDisplayName) const;
	void RemoteDisplayNameSet (const char *szName);
	inline EstiInterfaceMode RemoteInterfaceModeGet () const
		{
		  Lock ();
		  EstiInterfaceMode eRet = m_eRemoteInterfaceMode;
		  Unlock ();
		  return (eRet);
		};
	inline void RemoteInterfaceModeSet (EstiInterfaceMode eMode)
		{
		  Lock ();
		  m_eRemoteInterfaceMode = eMode;
		  Unlock ();
		};
	void RemoteDataClear ();
	inline const char *RemoteMacAddressGet ()  // todo:  Change to NOT return a pointer to our member var.
		{ return (m_szRemoteMacAddress); };
	inline void RemoteMacAddressSet (const char *szAddr)
		{
			Lock ();
			strncpy (m_szRemoteMacAddress, szAddr, sizeof (m_szRemoteMacAddress) -1);
			m_szRemoteMacAddress[sizeof (m_szRemoteMacAddress) - 1] = '\0';
			Unlock ();
		};
	virtual void RemoteNameGet (std::string *pRemoteName) const;
	void RemoteAlternateNameGet (std::string *pRemoteName) const;
	void RemoteAlternateNameSet (const char *pszRemoteAlternateName);

	virtual void RemoteProductNameGet (
		std::string *pRemoteProductName) const = 0;

	virtual void RemoteProductVersionGet (
		std::string *pRemoteProductVersion) const = 0;
	
	virtual void RemoteAutoSpeedSettingGet (
		EstiAutoSpeedMode *peAutoSpeedSetting) const = 0;

	inline const char *RemoteVcoCallbackGet ()  // todo:  Change to NOT return a pointer to our member var.
		 { return (m_szRemoteVcoCallback); };
	inline void RemoteVcoCallbackSet (const char *szNbr)
		{
			Lock ();
			strncpy (m_szRemoteVcoCallback, szNbr, sizeof (m_szRemoteVcoCallback) - 1);
			m_szRemoteVcoCallback[sizeof (m_szRemoteVcoCallback) - 1] = '\0';
			Unlock ();
		};
	EstiCallResult ResultGet () const;

	virtual EstiBool IsTransferableGet () const = 0;

	virtual EstiBool TextSupportedGet () const
		{return estiFALSE;};
	virtual void RemoteTextSupportedSet (EstiBool bRemoteTextSupported) {};
	virtual EstiBool DtmfSupportedGet () const
		{return estiFALSE;};
	
	inline EstiSwitch VideoPlaybackMuteGet ()
		{
		  Lock ();
		  EstiSwitch eRet = m_eVPMute;
		  Unlock ();
		  return (eRet);
		};
	inline void VideoPlaybackMuteSet (EstiSwitch eSwitch)
		{
		  Lock ();
		  m_eVPMute = eSwitch;
		  Unlock ();
		};
	inline int16_t VideoPlaybackSizeControllableGet ()
		{
		  Lock ();
		  int16_t n16Ret = m_n16VPSizeControllable;
		  Unlock ();
		  return (n16Ret);
		};
	inline void VideoPlaybackSizeControllableSet (int16_t nSizes)
		{
		  Lock ();
		  m_n16VPSizeControllable = nSizes;
		  Unlock ();
		};

	void VideoRecordSizeGet (uint32_t *pun32PixelsWide, uint32_t *pun32PixelsHigh) const;

	virtual EstiBool RemoteDeviceTypeIs (
		int nDeviceType) const = 0;
	virtual CstiProtocolManager  *ProtocolManagerGet () = 0;

	stiHResult BandwidthDetectionBegin ();

#ifdef stiHOLDSERVER
	virtual stiHResult HSEndService(EstiMediaServiceSupportStates eMediaServiceSupportState) = 0;
#endif

	EstiBool CallerBlocked (
		IstiBlockListManager2 *pBlockListManager);

	virtual void CallIdGet (std::string *pCallId);

	virtual void OriginalCallIDGet (std::string *pCallID);
	
	virtual void TextShareRemoveRemote() = 0;
	
	virtual void ConferencedSet (bool conferenced) = 0;
	virtual bool ConferencedGet () const = 0;
	
	virtual void connectingTimeStart () = 0;
	virtual void connectingTimeStop () = 0;
	virtual std::chrono::milliseconds connectingDurationGet () const = 0;
	
	virtual void connectedTimeStart () = 0;
	virtual void connectedTimeStop () = 0;
	virtual std::chrono::milliseconds  connectedDurationGet () const = 0;
	
	virtual std::chrono::seconds secondsSinceCallEndGet () const = 0;

	bool isAudioInitialized () const;

	CstiSignal<> videoRecordInitialized;

protected:

	CstiProtocolCall (
		const SstiConferenceParams *pstConferenceParams, EstiProtocol eProtocol);

	EstiBool AudioPlaybackActiveDetermine ();
	EstiBool TextPlaybackActiveDetermine ();
	EstiBool BandwidthDetectionComplete ();

	void FlowControlNeededDetermine ();
	virtual void FlowControlSend (
		IN EstiBool bUseOldData = estiFALSE,
		IN EstiBool bRemoteIsLegacyVP = estiTRUE);
	virtual void FlowControlSend (
		std::shared_ptr<CstiVideoPlayback> videoPlayback,
		int nNewRate) = 0;
	virtual void LostConnectionCheck (bool bDisconnectTimerStart) = 0;
	void StatsPrint ();
	virtual void VideoRecordDataStart () = 0;

	std::weak_ptr<CstiCall> m_poCstiCall;

	SstiConferenceParams m_stConferenceParams;

	std::shared_ptr<CstiAudioPlayback> m_audioPlayback;
	std::shared_ptr<CstiAudioRecord> m_audioRecord;
	std::shared_ptr<CstiTextPlayback> m_textPlayback;
	std::shared_ptr<CstiTextRecord> m_textRecord;
	std::shared_ptr<CstiVideoPlayback> m_videoPlayback;
	std::shared_ptr<CstiVideoRecord> m_videoRecord;

	int32_t 	m_callRate;				// The current call rate
	SstiCallStatistics m_stCallStatistics;

private:
	EstiSwitch m_eVPMute; // The video playback mute setting
	int16_t m_n16VPSizeControllable;// A bitmask representing which of
					// the remote systems capture sizes
					// are controllable.

	//
	// For call statistsics
	//
	// The following structure is used to maintain data about the current call.
	// An array of these structures will be kept and implemented as a circular
	// array.
	struct SstiCallStats
	{
		uint32_t un32APDataRecv = 0;		// Received AP data since previous update
		uint32_t un32APPktsLost = 0;		// Visable lost packets since previous update (after SRA)
		uint32_t un32APActualPktsLost = 0;	// Actual lost packets since previous update
		uint32_t un32APPktsRecv = 0;		// Number of packets received since previous  update
		uint32_t un32ARDataSent = 0;		// Transmitted AR data since previous update
		uint32_t un32VPDataRecv = 0;		// Received VP data since previous update
		uint32_t un32VPFramesRecv = 0;		// Received frames since previous update
		uint32_t un32VPPktsLost = 0;		// Lost packets since previous update
		uint32_t un32VPPktsRecv = 0;		// Number of packets received since previous update
		uint32_t un32VRDataSent = 0;		// Transmitted VR data since previous update
		uint32_t un32VRFramesSent = 0;		// Transmitted frames since previous update
		uint32_t un32TickCount = 0;		// Ticks since previous update
		uint32_t aun32PacketPositions[MAX_STATS_PACKET_POSITIONS] = {0};
		uint32_t un32MaxOutOfOrderPackets = 0;
		uint32_t  rtxVideoDataSent = 0; // Rtx data sent since last update.
		uint16_t  playbackDelay = 0;  // Delay we are adding to video.
		
		uint32_t rtcpJitter = 0;  // Total sum of jitter calculated on call used later to find average.
		uint32_t rtcpRTT = 0;  // Total sum of round trip time (RTT) calculated on call used later to find average.

		struct SstiCallStats* pNext = nullptr;	// Next structure in the buffer
		struct SstiCallStats* pPrev = nullptr;	// Previous structure in the buffer
	};

	SstiCallStats m_astCallStats[nSTATS_ELEMENTS];		// Array of call statistic structures
	SstiCallStats* m_pStatsElementHead;
	SstiCallStats* m_pCurrentStatsElement{nullptr};
	uint32_t m_un32PrevStatisticTickCount{0};				// Tick value when stats were last requested
	uint32_t m_un32SendConnectDetectAtTickCount;		// Time at which to send a connection detection message
	unsigned int m_unLostConnectionDelay;		// Delay between sending lost connection checks that increases over time.
	bool m_bDisconnectTimerStart;		// Determins if we start a timer to end the call.
	uint32_t m_un32AutoBandwidthAdjustStopTime;	// Time at which to discontinue bandwidth adjustment (in ticks)
	EstiBool m_bAutoBandwidthAdjustComplete; // Is Auto Bandwidth Adjustment complete or not
	uint32_t m_un32TimeOfFlowCtrlMsg;	// Time we sent a flow control message (in ticks)

	EstiInterfaceMode m_eRemoteInterfaceMode{estiSTANDARD_MODE};

	char m_szRemoteDisplayName[un8stiNAME_LENGTH + 1]{};
	char m_szRemoteAlternateName[un8stiNAME_LENGTH + 1]{};

	char m_szRemoteMacAddress[un8stiMAC_ADDR_LENGTH + 1]{};

	char m_szRemoteVcoCallback[un8stiVCO_CALLBACK_LENGTH + 1]{};

	EstiBool m_bIsHoldable; // Is this call holdable?  (depends on the remote system).

	EstiBool m_bAllowHangUp; // Flag indicating we can hang up the current call.

	mutable std::recursive_mutex m_lockMutex;
	int32_t m_abaTargetRate;  // The detected target rate for the remote endpoint.

	void AutoBandwidthAdjust ();

}; // End class CstiProtocolCall declaration

#endif // CSTIPROTOCOLCALL_H

