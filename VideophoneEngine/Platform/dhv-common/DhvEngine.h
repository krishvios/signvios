/*!
* \file DhvEngine.h
* \brief See DhvEngine.cpp
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2009 - 2019 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once
#include "IDhvEngine.h"
#include "CstiConferenceManager.h"
#include "CstiSignal.h"
#include "CstiRemoteLoggerService.h"
#include "stiSVX.h"

class DhvEngine : public IDhvEngine
{
public:
	DhvEngine ();
	~DhvEngine ();
	virtual stiHResult initialize(	ProductType productType,
									const char *version,
									const char *deviceModel,
									const char *osVersion,
									const std::string& sessionId,
									const SstiConferenceParams *conferenceParams,
									const SstiConferencePorts *conferencePorts) override;
	virtual stiHResult callObjectRemove(IstiCall * call) override;
	virtual int callObjectsCountGet() override;
	virtual stiHResult shutdown() override;
	virtual stiHResult reinvite (IstiCall * call) override;
	virtual void callDial (const std::string& URI,
			const std::string& hearingPhoneNumber,
			const OptString& localNameOverride,
			const std::string& remoteName,
			const std::string& relayLanguage,
			const std::string& uniqueId,
			std::shared_ptr<IstiCall>& call,
			const std::vector<SstiSipHeader>& additionalHeaders = {}) override;
	
	virtual void networkChangedNotify() override;

	// Logging
	virtual void remoteLogDeviceTokenSet (const std::string& deviceToken) override;
	virtual void remoteLogNetworkTypeSet (const std::string& networkType);
	virtual void remoteLogPhoneNumberSet(const std::string& phoneNumber);
	virtual void remoteLogEventSend (const std::string& message) override;

	virtual ISignal<std::shared_ptr<IstiCall>>& incomingCallSignalGet () override { return incomingCallSignal; }
	virtual ISignal<std::shared_ptr<IstiCall>>& outgoingCallSignalGet () override { return outgoingCallSignal; }
	virtual ISignal<std::shared_ptr<IstiCall>>& callEstablishingConferenceSignalGet () override { return callEstablishingConferenceSignal; }
	virtual ISignal<std::shared_ptr<IstiCall>>& callConferencingSignalGet () override { return callConferencingSignal; }
	virtual ISignal<std::shared_ptr<IstiCall>>& disconnectedCallSignalGet () override { return disconnectedCallSignal; }
	
	virtual ISignal<uint32_t>& localRingSignalGet () override { return conferenceManager->localRingCountSignal; }
	virtual ISignal<int>& remoteRingSignalGet () override { return conferenceManager->remoteRingCountSignal; }
	
	virtual ISignal<CstiCallSP>& encryptionStatusChangedSignalGet () override { return conferenceManager->conferenceEncryptionStateChangedSignal; }
	
	virtual ISignal<CstiCallSP>& callTransferringSignalGet () override { return conferenceManager->callTransferringSignal; }
	
	std::string PublicIPAddressGet();
	
	virtual ISignal<std::string>& publicIPAddressSignalGet () { return conferenceManager->publicIpResolvedSignal; }
	void enableRemoteCallStatsLogging(int remoteCallStatsLoggingValue);
	void updateSipDomain(std::string sipDomain);

	bool UserEndedCall;
	bool StoreMode;
	std::string networkType;

private:
	stiHResult initializeHTTP ();
	stiHResult initializeLoggingService(ProductType productType,
										const std::string &version);
	void CallStatusSend(CstiCallSP call, CstiSipCallLegSP callLeg);
	std::string ICENominationsResultStringGet (EstiICENominationsResult eICENominationsResult);
	std::string RemoteIceSupportStringGet (IceSupport iceSupport);
	std::string MediaTransportStringGet (EstiMediaTransportType eTransportType);

private:
	CstiConferenceManager* conferenceManager;
	CstiSignalConnection::Collection m_signalConnections;
	SstiUserPhoneNumbers userPhoneNumbers = {};
	std::string deviceModel;
	std::string osVersion;
	
	//Signals
	CstiSignal<std::shared_ptr<IstiCall>> incomingCallSignal;
	CstiSignal<std::shared_ptr<IstiCall>> outgoingCallSignal;
	CstiSignal<std::shared_ptr<IstiCall>> callEstablishingConferenceSignal;
	CstiSignal<std::shared_ptr<IstiCall>> callConferencingSignal;
	CstiSignal<std::shared_ptr<IstiCall>> disconnectedCallSignal;

	CstiHTTPTask *httpLoggerTask = nullptr;
	CstiRemoteLoggerService *remoteLoggerService = nullptr;
};

