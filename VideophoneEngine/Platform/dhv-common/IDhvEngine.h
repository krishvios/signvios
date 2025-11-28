/*!
* \file IDhvEngine.h
* \brief An Interface for the DHV Engine
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2009 - 2019 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once
#include "stiSVX.h"
#include "stiConfDefs.h"
#include "IstiCall.h"
#include "ISignal.h"
#include "OptString.h"

//Constants
static const int T35_COUNTRY_CODE = 181;  // United States of America
static const int T35_MFG_CODE = 21334;    // Sorenson Communications' assigned code
static const int MaxCalls = 1; //Maximum number of concurent calls.

//
// Forward Declarations
//
class IDhvEngine;

/*
	Create an instance of the Media Server Engine.
*/
IDhvEngine * CreateDhvEngine();

void DestroyDhvEngine(IDhvEngine * engine);

class IDhvEngine
{
public:
	/*!
	// \brief Initializer for Dhv Engine
	//
	// \param productName the name of the product
	// \param version the version of the product
	// \param conferenceParams the configuration of the conference manager
	// \param conferencePorts the ports used by the conference manager
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult initialize(	ProductType productType,
									const char *version,
									const char *deviceModel,
									const char *osVersion,
									const std::string& sessionId,
									const SstiConferenceParams *conferenceParams,
									const SstiConferencePorts *conferencePorts) = 0;

	/*!
	// \brief Removes a call object, should be called when the UI is finished with the call object
	//
	// \param call: the call object to remove
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult callObjectRemove(IstiCall * call) = 0;

	/*!
	// \brief Returns the number of call objects in the conference manager.
	//
	// \return Number of Call Objects
	*/
	virtual int callObjectsCountGet() = 0;

	/*!
	// \brief Shutsdown the conference manager waiting for it to finish shutting down.
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult shutdown() = 0;

	/*!
	// \brief Reinvite a Call
	//
	// \return stiRESULT_SUCCESS
	*/
	virtual stiHResult reinvite (IstiCall * call) = 0;

	/*!
	// \brief Dial a uri
	*/
	virtual void callDial (const std::string& URI,
			const std::string& hearingPhoneNumber,
			const OptString& localNameOverride,
			const std::string& remoteName,
			const std::string& relayLanguage,
			const std::string& uniqueId,
			std::shared_ptr<IstiCall>& call,
			const std::vector<SstiSipHeader>& additionalHeaders) = 0;
	
	/*!
	// \brief Notify the engine that the network has changed so it can reconfigure its connections.
	*/
	virtual void networkChangedNotify() = 0;

	virtual void remoteLogDeviceTokenSet (const std::string& deviceToken) = 0;
	virtual void remoteLogEventSend (const std::string& message) = 0;

	virtual ISignal<std::shared_ptr<IstiCall>>& incomingCallSignalGet () = 0;

	virtual ISignal<std::shared_ptr<IstiCall>>& outgoingCallSignalGet () = 0;
	
	virtual ISignal<std::shared_ptr<IstiCall>>& callEstablishingConferenceSignalGet () = 0;
	
	virtual ISignal<std::shared_ptr<IstiCall>>& callConferencingSignalGet () = 0;

	virtual ISignal<std::shared_ptr<IstiCall>>& disconnectedCallSignalGet () = 0;
	
	virtual ISignal<uint32_t>& localRingSignalGet () = 0;
	
	virtual ISignal<int>& remoteRingSignalGet () = 0;
	
	virtual ISignal<CstiCallSP>& encryptionStatusChangedSignalGet () = 0;
    
    virtual ISignal<CstiCallSP>& callTransferringSignalGet () = 0;
};


