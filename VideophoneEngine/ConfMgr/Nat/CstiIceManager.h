////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: CstiIceManager
//
//  File Name:  CstiIceManager.h
//
//  Abstract:
//      See CstiIceManager.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIICEMANAGER_H
#define CSTIICEMANAGER_H


//
// Includes
//
#include <vector>
#include <string>
#include "stiDefs.h"
#include "RvIceMgr.h"
#include "stiConfDefs.h"
#include "CstiMediaTransports.h"
#include "CstiIceSession.h"
#include "CstiSignal.h"

#include <boost/optional.hpp>

//
// Forward Declarations
//
class CstiIceSession;

//
// Class Declaration
//
class CstiIceManager
{
public:

	// Methods
	CstiIceManager () = default;

	virtual ~CstiIceManager () = default;

	stiHResult IceStackInitialize (int maxIceSessions);

	stiHResult IceStackUninitialize ();

	stiHResult ServersSet (
		const boost::optional<CstiServerInfo> &server,
		const boost::optional<CstiServerInfo> &serverAlt,
		CstiIceSession::Protocol protocol);
	
	void PrioritizePrivacySet (bool prioritizePrivacy) { m_prioritizePrivacy = prioritizePrivacy; }

	stiHResult SessionStart (
		CstiIceSession::EstiIceRole eRole,
		const CstiSdp &Sdp,
		IceSupport *iceSupport,
		size_t CallbackParam,
		const std::string &PublicIPv4,
		const std::string &PublicIPv6,
		CstiIceSession::Protocol protocol,
		CstiIceSession **ppIceSession);

	stiHResult IceSessionTryAnother (
		std::string PublicIPv4,
		std::string PublicIPv6,
		size_t CallbackParam,
		CstiIceSession::Protocol protocol,
		CstiIceSession **ppIceSession);

	CstiIceSession::Protocol DefaultProtocolGet () const;

	void rvLogManagerSet (RvLogMgr* logManager) { m_logManager = logManager; }

	// Each signal uses a callLeg id as the parameter
	CstiSignal<size_t> iceGatheringCompleteSignal;
	CstiSignal<size_t> iceGatheringTryingAnotherSignal;
	// Parameters: calllegId and success
	CstiSignal<size_t, bool> iceNominationsCompleteSignal;

	void useLiteModeSet (bool useLiteMode);

	void rejectHostOnlyRequestsSet (bool rejectOnlyRequests);

private:

	CstiIceSession::Protocol m_defaultProtocol;
	boost::optional<CstiServerInfo> m_server;
	boost::optional<CstiServerInfo> m_serverAlt;
	bool m_prioritizePrivacy{false};
	RvIceStackHandle m_hStackHandle{nullptr};
	RvLogMgr* m_logManager{};
	bool m_useLiteMode = false;
	bool m_rejectHostOnlyRequests = false; ///< Reject an ICE session which only has candidates of type "host" which come from private ip ranges

	stiHResult AccountCreateSession (
		CstiIceSession::EstiIceRole eRole,
		size_t CallbackParam,
		CstiIceSession **ppIceSession);

	stiHResult AccountStartSession (
		CstiIceSession *pIceSession,
		const CstiSdp &Sdp,
		IceSupport *iceSupport,
		const std::string &PublicIPv4,
		const std::string &PublicIPv6,
		CstiIceSession::Protocol protocol);

	static void RVCALLCONV IceSessionCompleteCB (
		RvIceSessionHandle hSession, ///< Handle to the ICE session object.
		RvIceAppSessionHandle hAppSession, ///< Handle to the application session object.
		RvIceSessionCompleteResult complResult); ///< Indicates the application how the ICE processing was completed.

	static RvStatus RVCALLCONV IceSessionCandidatesGatheringCompletedCB (
		RvIceSessionHandle hSession, ///< Handle to the ICE session object.
		RvIceAppSessionHandle hAppSession, ///< Handle to the application session object.
		RvIceReason eReason); ///< Indicates how the gathering was completed.

	static RvBool RVCALLCONV TurnAllocationCB (
		RvIceSessionHandle hSession, ///< Handle to the ICE session object.
		RvIceAppSessionHandle hAppSession, ///< Handle to the application session object.
		RvIceCandidateHandle hCandidate,  ///< Handle to the ICE candidate object.
		const RvAddress *pAddress,  ///< The candidate’s local address.
		RvBool bIsTurn,
		RvBool bTurnAllocSuccess,  ///< Indicates whether the TURN allocation was established successfully.
		RvStunTransactionError eStunTransactionError, ///< Any associated connection failure code.
		RvInt eStunResponseError); ///< Any associated failure remote response.

	stiHResult IceCompleted (CstiIceSession *pIceSession);
};

#endif // CSTIICEMANAGER_H
// end file CstiIceManager.h
