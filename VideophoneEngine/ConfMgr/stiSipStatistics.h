////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Structure Name: SstiRvSipStats
//
//  File Name:  stiSipStatistics.h
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTISIPSTATISTICS_H
#define CSTISIPSTATISTICS_H

//
// Forward Declarations
//

//
// Includes
//
#include "stiSVX.h"
#include <ostream>
#include <fstream>
#include <sstream>
#include "JsonSerializer.h"

struct SstiRvUsage : public vpe::Serialization::JsonSerializable
{
	operator Poco::JSON::Object () const override;

	uint32_t un32NumOfAllocatedElements{0};
	uint32_t un32NumOfUsedElements{0};
	uint32_t un32MaxUsageOfElements{0};

};

#define SERIALIZE_SIP_STAT(A) output << #A << ".Allocated="	<< stStats.A.un32NumOfAllocatedElements << "\n"; \
	output << #A << ".InUse=" << stStats.A.un32NumOfUsedElements << "\n"; \
	output << #A << ".HighWater=" << stStats.A.un32MaxUsageOfElements << "\n";


struct SstiRvSipStats : public vpe::Serialization::JsonSerializable
{
	operator Poco::JSON::Object () const override;

	// Information about resources used by the Call-Leg module
	SstiRvUsage CallLegCalls;			// The resource information of the call-leg objects.
	SstiRvUsage CallLegTrnsLst;		// Every call-leg holds one list of 'transaction handle' objects
												// to manage all its related transactions. This is the resource
												// information of all the allocated transactions lists.
	SstiRvUsage CallLegTrnsHndl;		// This is the resource information of all the 'transaction handle'
												// objects that are allocated by all call-legs
	SstiRvUsage CallLegInvtLst;		// Every call-leg holds one list of 'invite' objects
												// to manage all its invite transactions. This is the resource
												// information of all the allocated invite lists.
	SstiRvUsage CallLegInvtObjs;		// This is the resource information of all the 'invite'
												// objects that are allocated by all call-legs.

	// Information about resources used by the mid-layer
	SstiRvUsage MidLayUserFds;			// The resources status of the Mid file descriptors.
	SstiRvUsage MidLayUserTimrs;		// The resources status of the Mid timers.

	// Information about resources used by the Publish-Client module
	SstiRvUsage PublishClient;			// The resource information of the publish-client objects.

	// Information about resources used by the Register-Client module
	SstiRvUsage RegisterClient;		// The resource information of the register-client objects.

	// Information about resources used by the Resolver module
	SstiRvUsage Resolver;				// The resource information of the resolver objects.

	// Information about RPOOL resources
	SstiRvUsage RPool;					// The resource information of the rpool objects.

	// Information about resources used by the Stack Manager module
	/*
	 * This contains the resource information of the three memory pools that are used
	 * by the SIP Stack: msgPoolElements, generalPoolElements and headerPoolElements.
	 * The difference between the three memory pools is in the size of memory it supplies.
	 * It also contains the information of the timers that are used by the entire SIP Stack.
	 */
	SstiRvUsage StkMgrMsgPoolEl;
	SstiRvUsage StkMgrGenPoolEl;
	SstiRvUsage StkMgrHdrPoolEl;
	SstiRvUsage StkMgrTmrPool;

	// Information about resources used by the Subscription module
	SstiRvUsage SubsSubscriptns;	// The resource information of the subscription objects.
	SstiRvUsage SubsNotificatns;	// The resource information of the notify objects allocated by all subscriptions.
	SstiRvUsage SubsNotifyLists;	// Each subscription holds one list of notify objects to manage all its notify
											// objects. This is the resource information of all the allocated notify lists.

	// Information about resources used by the Transaction module
	SstiRvUsage Transaction;		// The resource information of the transaction objects.

	// Information about resources used by the Tranmitter module
	SstiRvUsage Transmitter;		// The resource information of the transmitter objects.

	// Information about resources used by the Transport module
	SstiRvUsage TrnsprtCon;			// The resource information of the connection objects.
	SstiRvUsage TrnsprtConNoOwn;	// The resource information of the allocated connection which
											// does not have owners.
	SstiRvUsage TrnsprtpQueEl;		// The resource information of the allocated queue elements. This
											// queue managees all the non-synchronious actions.
	SstiRvUsage TrnsprtpQueEvnt;	// The resource information of the allocated queue event elements. This
											// queue manages all the non-synchronious actions.
	SstiRvUsage TrnsprtRdBuff;		// The resource information of the allocated read-buffers.
	SstiRvUsage TrnsprtConHash;	// The resource information of the connection hash table.
	SstiRvUsage TrnsprtOwnHash;	// The resource information of the connection owners hash table.
	SstiRvUsage TrnsprtTlsSess;	// The resource information of the TLS session objects.
	SstiRvUsage TrnsprtTlsEng;		// The resource information of the TLS engine objects.
	SstiRvUsage TrnsprtOOResEv;  	// The resource information of the allocated out-of-resource event elements pool.
											// This pool manages all the pqueue events that failed to be processed because of an
											// out-of-resource situation.

#ifdef RV_SIP_STATISTICS_ENABLED
	// SIP Stack Statistics  (These can be reset in the SIP stack)
	uint32_t un32RcvdInvite;
	uint32_t un32RcvdInviteRetrans;
	uint32_t un32RcvdNonInviteReq;
	uint32_t un32RcvdNonInviteReqRetrans;
	uint32_t un32RcvdResponse;
	uint32_t un32RcvdResponseRetrans;
	uint32_t un32SentInvite;
	uint32_t un32SentInviteRetrans;
	uint32_t un32SentNonInviteReq;
	uint32_t un32SentNonInviteReqRetrans;
	uint32_t un32SentResponse;
	uint32_t un32SentResponseRetrans;
#endif
	
	friend std::ostream& operator << (std::ostream& output, SstiRvSipStats &stStats);

};


#endif // CSTISIPSTATISTICS_H
