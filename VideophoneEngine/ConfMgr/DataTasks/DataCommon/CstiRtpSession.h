/*!
* \file CstiRtpSession.h
* \brief Represents an RTP/RTCP media session that can be shared
*        by multiple components (eg VR and VP)
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
*/

#pragma once

#include "CstiSignal.h"
#include "CstiMediaTransports.h"
#include "CstiAesEncryptionContext.h"
#include "SDESKey.h"
#include "DtlsContext.h"
#include "rvnettypes.h"
#include "rtp.h"
#include "rvrtcpfb.h"
#include "rvsrtpkeyexchange.h"

#include <mutex>
#include <functional>

namespace vpe
{



/*!
 * \brief describes information about 4 packet streams
 *
 * ie: incoming rtp/rtcp and outgoing rtp/rtcp?
 *
 * NOTE: the srtp support in this class was based on radvision
 * example code
 */
class RtpSession
{
public:
	struct SessionParams
	{
		int portBase = 0;
		int portRange = 0;
		bool tunneled = false;
		bool ipv6Enabled = false;

		// Indicates whether we should use FIR/PLI/TMMBR
		// If true, this means that:
		//  - remote party supports the mechanism
		//  - was not forcibly disabled via the property table
		bool rtcpFeedbackFirEnabled = false;   // Used to request an IDR keyframe
		bool rtcpFeedbackPliEnabled = false;   // Used to request an Intra frame due to picture loss (could be IDR or not)
		bool rtcpFeedbackAfbEnabled = false;   // Receive only (not negotiated) - Used to listen for REMB from WebRTC endpoints
		bool rtcpFeedbackTmmbrEnabled = false; // Used to request temporary rate changes to sender
		bool rtcpFeedbackNackRtxEnabled = false; // Used to signal lost packets and retransmit missing data

		bool srtpEncrypted = false;
		SDESKey encryptKey;
		SDESKey decryptKey;

		bool rtcpStreamAdded = false;
	};


public:
	RtpSession (
			const SessionParams &sessionParams);

	~RtpSession ();

	RtpSession (const RtpSession &other) = delete;
	RtpSession (RtpSession &&other) = delete;
	RtpSession &operator= (const RtpSession &other) = delete;
	RtpSession &operator= (RtpSession &&other) = delete;

	stiHResult open ();
	stiHResult close ();
	bool isOpen ();

	stiHResult hold ();
	stiHResult resume ();

	RvRtpSession rtpGet () const;
	RvRtcpSession rtcpGet () const;

	RvTransport rtpTransportGet () const;
	RvTransport rtcpTransportGet () const;

	stiHResult transportsSet (
		const CstiMediaTransports &transports);

	void remoteAddressSet (
			const RvAddress *rtpAddress,
			const RvAddress *rtcpAddress,
			bool *addressChanged);
	
	bool remoteAddressCompare (
			const RvAddress &rtpAddress,
			const RvAddress &rtcpAddress);

	stiHResult RemoteAddressGet (
		std::string *rtpAddress,
		int *rtpPort,
		std::string *rtcpAddress,
		int *rtcpPort);

	stiHResult LocalAddressGet (
		std::string *rtpAddress,
		int *rtpPort,
		std::string *rtcpAddress,
		int *rtcpPort);

	unsigned int rtcpRemoteIpGet () const;
	void rtcpRemoteIpSet (unsigned int ipAddress);

	unsigned int rtcpRemotePortGet () const;
	void rtcpRemotePortSet (unsigned int port);

	bool tunneledGet () const;
	void tunneledSet (bool tunneled);

	bool rtcpFeedbackFirEnabledGet () const;
	void rtcpFeedbackFirEnabledSet (bool enabled);
	bool rtcpFeedbackPliEnabledGet () const;
	void rtcpFeedbackPliEnabledSet (bool enabled);
	bool rtcpFeedbackAfbEnabledGet () const;
	void rtcpFeedbackAfbEnabledSet (bool enabled);
	bool rtcpFeedbackTmmbrEnabledGet () const;
	void rtcpFeedbackTmmbrEnabledSet (bool enabled);
	bool rtcpFeedbackNackRtxEnabledGet () const;
	void rtcpFeedbackNackRtxEnabledSet (bool enabled);

	void rtcpFeedbackFirEventHandlerSet (RvRtcpFbEventFIR firEventHandler);
	void rtcpFeedbackPlilEventHandlerSet (RvRtcpFbEventPLI pliEventHandler);
	void rtcpFeedbackAfbEventHandlerSet (RvRtcpFbEventAFB afbEventHandler);
	void rtcpFeedbackTmmbrEventHandlerSet (RvRtcpFbEventTMMBR tmmbrEventHandler);
	void rtcpFeedbackNackEventHandlerSet (RvRtcpFbEventNACK nackEventHandler);

	void SDESKeysSet (const vpe::SDESKey &encryptKey, const vpe::SDESKey &decryptKey);
	bool decryptKeyChanged (const vpe::SDESKey &decryptKey);
	void createNewSsrc ();

	void dtlsContextSet(std::shared_ptr<DtlsContext> dtlsContext);
	void dtlsBegin ();
	void onDtlsCompleteCallbackSet (const std::function<void ()>& callback);

	void rtcpSrtpStreamAddedSet();

	RtpSession::SessionParams paramsGet () const;

	int RtpRead(void *buf, int len, RvRtpParam *p);

	void rtpEventHandlerSet (RvRtpEventHandler_CB rtpEventHandler, void* context);

	EncryptionState encryptionStateGet () { return m_encryptionState; }

	CstiSignal<EncryptionState> encryptionStateChangedSignal;

	KeyExchangeMethod keyExchangeMethodGet () { return m_keyExchangeMethod; }
	void encryptionDisabled ();

public:
#ifdef stiTUNNEL_MANAGER
	static RvRtpTransportProvide m_rtpTransportProvide;
#endif


private:

	void remoteAddressSet (
			const RvAddress *rtpAddress,
			const RvAddress *rtcpAddress);

	void destinationContextAdd ();
	void destinationContextRemove ();

	stiHResult openFromTransports ();
	stiHResult openFromPorts ();

	static void rtpTransportProvideCallbackSet (
			RvRtpTransportProvideCB callback);

	void constructRtcpFeedbackPlugin ();
	void destructRtcpFeedbackPlugin ();

	static void rtpDataThrowAwayCallback (
		RvRtpSession rtp,
		void *callbackParam);

	void encryptionStateUpdateFromAesInitialized (int KeyBitSize);

	stiHResult dtlsSrtpAesEncryptionBegin ();
	stiHResult sdesSrtpAesEncryptionBegin ();
	stiHResult srtpAesEncryptionEnd ();

	static void srtpAesDataConstructCB (
			RvSrtpAesPlugIn *plugin,
			RvSrtpEncryptionPurpose purpose);

	static void srtpAesDataDestructCB (
			RvSrtpAesPlugIn *plugin,
			RvSrtpEncryptionPurpose purpose);

	static RvBool srtpAesInitializeECBModeCB (
			RvSrtpAesPlugIn *plugin,
			RvSrtpEncryptionPurpose purpose,
			RvUint8 direction,
			const RvUint8 *key,
			RvUint16 keyBitSize,
			RvUint16 blockBitSize);

	static RvBool srtpAesInitializeCBCModeCB (
			RvSrtpAesPlugIn *plugin,
			RvSrtpEncryptionPurpose purpose,
			RvUint8 direction,
			const RvUint8 *key,
			RvUint16 keyBitSize,
			RvUint16 blockBitSize,
			const RvUint8 *iv);

	static void srtpAesEncryptCB (
			RvSrtpAesPlugIn *plugin,
			RvSrtpEncryptionPurpose purpose,
			const RvUint8 *srcBuffer,
			RvUint8 *dstBuffer,
			RvUint32 length);

	static void srtpAesDataDecryptCB (
			RvSrtpAesPlugIn *plugin,
			RvSrtpEncryptionPurpose purpose,
			const RvUint8 *srcBuffer,
			RvUint8 *dstBuffer,
			RvUint32 length);

	static RvBool dtlsRtpReadyCB (
		RvDtlsSrtpSession* self,
		RvRtpSession hRtp,
		void* ctx);

#ifdef stiTUNNEL_MANAGER
	static void RVCALLCONV rtpTransportProvideCallback (
			void *appContext,
			RvUint32 rtpRtcp,
			RvUint32 timing,
			RvTransport transport,
			RvBool *bContinue);
#endif

	static std::string uniqueMkiGet();

	void rtcpFbEventHandlerSetSafely();

private:
	SessionParams m_params;

	// multiple threads use the same session object, this protects all access
	mutable std::recursive_mutex m_mutex;

	RvRtpSession m_rtp = nullptr;
	RvRtcpSession m_rtcp = nullptr;
	CstiMediaTransports m_transports;

	RvRtpEventHandler_CB m_rtpEventHandler{};
	void* m_rtpEventHandlerContext{};

	RvAddress m_remoteRtpAddress{};   // Address information for remote end
	RvAddress m_remoteRtcpAddress{};

	// How are these different than the above?
	unsigned int m_rtcpRemoteIp = 0;
	unsigned int m_rtcpRemotePort = 0;

	bool m_rtcpFeedbackPluginConstructed = false;
	RvRtcpFbEvent m_defaultFeedbackHandlers = {};
	RvRtcpFbEvent m_feedbackHandlers = {};

	CstiAesEncryptionContext encryptionContexts[RvSrtpEncryptionPurposes];
	bool m_srtpAesPluginStarted = false;
	RvSrtpAesPlugIn m_srtpAesPlugin {};

	EncryptionState m_encryptionState{};
	KeyExchangeMethod m_keyExchangeMethod{};

	int64_t m_ssrc {-1};

	std::shared_ptr<vpe::DtlsContext> m_dtlsContext;
	RvDtlsSrtpSession* m_dtlsSrtpSession{};
	std::function<void ()> m_onDtlsCompleteCallback;
	bool m_dtlsRenegotiationRequired{false};
	std::list<int64_t> m_encryptedSSRC{};
};

} // namespace
