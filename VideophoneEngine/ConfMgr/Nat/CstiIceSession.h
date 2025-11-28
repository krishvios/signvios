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
#ifndef CSTIICESESSION_H
#define CSTIICESESSION_H

#include <memory>
#include <vector>
#include <boost/optional.hpp>
#include "stiDefs.h"
#include "RvIceMgr.h"
#include "CstiMediaTransports.h"
#include "CstiSdp.h"

//
// Forward Declarations
//
class CstiIceManager;


/*!\brief Structure in which the ICE attributes are stored.
 *
 *
 */
struct SstiIceSdpAttributes
{
	SstiIceSdpAttributes () = default;

	~SstiIceSdpAttributes ()
	{
		if (pMsgAttributes)
		{
			rvSdpMsgDestruct (pMsgAttributes);
			pMsgAttributes = nullptr;
		}

		for (unsigned int i = 0; i < MediaAttributes.size (); i++)
		{
			if (MediaAttributes[i])
			{
				rvSdpMsgDestruct (MediaAttributes[i]);
				MediaAttributes[i] = nullptr;
			}
		}
	}

	RvSdpMsg *pMsgAttributes{nullptr};
	std::vector<RvSdpMsg *> MediaAttributes;
};


class CstiIceSession
{
public:

	enum ICESessionState
	{
		estiIdle,
		estiGathering,
		estiGatheringComplete,
		estiNominating,
		estiComplete,
		estiNominationsFailed,
	};

	enum EstiIceRole
	{
		estiICERoleOfferer,
		estiICERoleAnswerer,
		estiICERoleGatherer
	};

	/*! \brief settings for gathering candidates
	 */
	enum class Protocol
	{
		None,
		Stun,
		Turn,
	};

	CstiIceSession ();

	CstiIceSession (
		const CstiIceSession &Other);

	~CstiIceSession ();

	stiHResult SessionProceed ();

	stiHResult SessionEnd ();

	stiHResult SdpUpdate (
		CstiSdp *pSdp);

	void SdpReceived (
		const CstiSdp &Sdp,
		IceSupport *iceSupport);

	stiHResult SdpIceSupportAdd (
		const CstiSdp &Sdp);

	stiHResult RelatedSessionStart (
		size_t CallbackParam,
		CstiIceSession **ppRelatedSession);

	stiHResult NominationsGet (
		CstiMediaTransports *pLocalAudio,
		CstiMediaTransports *pLocalText,
		CstiMediaTransports *pLocalVideo,
		CstiMediaAddresses *pRemoteAudio,
		CstiMediaAddresses *pRemoteText,
		CstiMediaAddresses *pRemoteVideo,
		SstiIceSdpAttributes *pAttributes);

	stiHResult DefaultTransportsGet (
		const CstiSdp &SdpMsg,
		CstiMediaTransports *pAudio,
		CstiMediaTransports *pText,
		CstiMediaTransports *pVideo);

	stiHResult LocalAddressesGet (
		RvAddress *pAudioRtp,
		RvAddress *pAudioRtcp,
		RvAddress *pTextRtp,
		RvAddress *pTextRtcp,
		RvAddress *pVideoRtp,
		RvAddress *pVideoRtcp);

	stiHResult ServerReflexivePortsAdd (
		const std::string &PublicIP,
		int nAddressType);

	stiHResult CandidatesList ();
	
	static stiHResult RedactCandidateRaddr (RvSdpAttribute *attr);

	bool AllRemoteCandidatesArePrivateHost ();

	ICESessionState m_eState{estiIdle};
	bool m_bRelatedSession{false};
	size_t m_CallbackParam{0};
	RvIceSessionHandle m_hSession{nullptr};
	EstiIceRole m_eRole{estiICERoleOfferer};
	CstiIceManager *m_pIceManager{nullptr};
	RvIceSessionCompleteResult m_CompleteResult;
	CstiSdp m_originalSdp;
	CstiSdp m_remoteSdp;
	RvIceStunServerCfg m_serverCfg{};
	RvIceStackHandle m_hStackHandle{nullptr};
	bool m_bTurnFailureDetected{false};
	boost::optional< std::vector<std::string> > m_turnIpList;
	uint16_t m_un16MediaPortBase{0};
	uint16_t m_un16MediaPortRange{0};
	std::string m_PublicIPv4;
	std::string m_PublicIPv6;
	bool m_bRetrying{false};
	bool m_prioritizePrivacy{false};

	// Remove once IceNominationsApply in CstiSipManager::ConnectedCallProcess can be safely removed
	bool m_nominationsApplied{false};

private:

};

const char *IceRoleNameGet (
	CstiIceSession::EstiIceRole eRole);

#endif // CSTIICESESSION_H
