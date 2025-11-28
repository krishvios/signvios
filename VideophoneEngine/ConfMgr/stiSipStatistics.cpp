////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Structure Name: SstiRvSipStats
//
//  File Name:  stiSipStatistics.cpp
//
//  Abstract:  This structure is for aggrigating multiple data points from the Sip Stack into
//     a single structure.  It also offers a operator<< method to serialize the data into a std::ostream.
//
////////////////////////////////////////////////////////////////////////////////
#include "stiSipStatistics.h"

SstiRvUsage::operator Poco::JSON::Object () const
{
	vpe::Serialization::JsonSerializer serializer;

	serializer.serialize("allocatedElements", un32NumOfAllocatedElements);
	serializer.serialize("usedElements", un32NumOfUsedElements);
	serializer.serialize("maximumElements", un32MaxUsageOfElements);

	return serializer;
}

#define SERIALIZE_DATA(d) (	serializer.serialize(#d, d))

SstiRvSipStats::operator Poco::JSON::Object () const
{
	vpe::Serialization::JsonSerializer serializer;

	SERIALIZE_DATA (CallLegCalls);
	SERIALIZE_DATA (CallLegTrnsLst);
	SERIALIZE_DATA (CallLegTrnsHndl);
	SERIALIZE_DATA (CallLegInvtLst);
	SERIALIZE_DATA (CallLegInvtObjs);
	SERIALIZE_DATA (MidLayUserFds);
	SERIALIZE_DATA (MidLayUserTimrs);
	SERIALIZE_DATA (PublishClient);
	SERIALIZE_DATA (RegisterClient);
	SERIALIZE_DATA (Resolver);
	SERIALIZE_DATA (RPool);
	SERIALIZE_DATA (StkMgrMsgPoolEl);
	SERIALIZE_DATA (StkMgrGenPoolEl);
	SERIALIZE_DATA (StkMgrHdrPoolEl);
	SERIALIZE_DATA (StkMgrTmrPool);
	SERIALIZE_DATA (SubsSubscriptns);
	SERIALIZE_DATA (SubsNotificatns);
	SERIALIZE_DATA (SubsNotifyLists);
	SERIALIZE_DATA (Transaction);
	SERIALIZE_DATA (Transmitter);
	SERIALIZE_DATA (TrnsprtCon);
	SERIALIZE_DATA (TrnsprtConNoOwn);
	SERIALIZE_DATA (TrnsprtpQueEl);
	SERIALIZE_DATA (TrnsprtpQueEvnt);
	SERIALIZE_DATA (TrnsprtRdBuff);
	SERIALIZE_DATA (TrnsprtConHash);
	SERIALIZE_DATA (TrnsprtOwnHash);
	SERIALIZE_DATA (TrnsprtTlsSess);
	SERIALIZE_DATA (TrnsprtTlsEng);
	SERIALIZE_DATA (TrnsprtOOResEv);

	return serializer;
}

std::ostream& operator << (std::ostream &output, SstiRvSipStats &stStats)
{

	SERIALIZE_SIP_STAT (CallLegCalls);
	SERIALIZE_SIP_STAT (CallLegTrnsLst);
	SERIALIZE_SIP_STAT (CallLegTrnsHndl);
	SERIALIZE_SIP_STAT (CallLegInvtLst);
	SERIALIZE_SIP_STAT (CallLegInvtObjs);
	SERIALIZE_SIP_STAT (MidLayUserFds);
	SERIALIZE_SIP_STAT (MidLayUserTimrs);
	SERIALIZE_SIP_STAT (PublishClient);
	SERIALIZE_SIP_STAT (RegisterClient);
	SERIALIZE_SIP_STAT (Resolver);
	SERIALIZE_SIP_STAT (RPool);
	SERIALIZE_SIP_STAT (StkMgrMsgPoolEl);
	SERIALIZE_SIP_STAT (StkMgrGenPoolEl);
	SERIALIZE_SIP_STAT (StkMgrHdrPoolEl);
	SERIALIZE_SIP_STAT (StkMgrTmrPool);
	SERIALIZE_SIP_STAT (SubsSubscriptns);
	SERIALIZE_SIP_STAT (SubsNotificatns);
	SERIALIZE_SIP_STAT (SubsNotifyLists);
	SERIALIZE_SIP_STAT (Transaction);
	SERIALIZE_SIP_STAT (Transmitter);
	SERIALIZE_SIP_STAT (TrnsprtCon);
	SERIALIZE_SIP_STAT (TrnsprtConNoOwn);
	SERIALIZE_SIP_STAT (TrnsprtpQueEl);
	SERIALIZE_SIP_STAT (TrnsprtpQueEvnt);
	SERIALIZE_SIP_STAT (TrnsprtRdBuff);
	SERIALIZE_SIP_STAT (TrnsprtConHash);
	SERIALIZE_SIP_STAT (TrnsprtOwnHash);
	SERIALIZE_SIP_STAT (TrnsprtTlsSess);
	SERIALIZE_SIP_STAT (TrnsprtTlsEng);
	SERIALIZE_SIP_STAT (TrnsprtOOResEv);

#ifdef RV_SIP_STATISTICS_ENABLED
	output << "RcvdInvites=" << stStats.un32RcvdInvite << "\n";
	output << "RcvdInviteRetrans=" << stStats.un32RcvdInviteRetrans << "\n";
	output << "RcvdNonInviteRequests=" << stStats.un32RcvdNonInviteReq << "\n";
	output << "RcvdNonInviteRequestsRet=" << stStats.un32RcvdNonInviteReqRetrans << "\n";
	output << "RcvdResponse=" << stStats.un32RcvdResponse << "\n";
	output << "RcvdResponseRetrans=" << stStats.un32RcvdResponseRetrans << "\n";
	output << "TxInvites=" << stStats.un32SentInvite << "\n";
	output << "TxInviteRetrans=" << stStats.un32SentInviteRetrans << "\n";
	output << "TxNonInviteReq=" << stStats.un32SentNonInviteReq << "\n";
	output << "TxNonInviteReqRetrans=" << stStats.un32SentNonInviteReqRetrans << "\n";
	output << "TxResponse=" << stStats.un32SentResponse << "\n";
	output << "TxResponseRetrans=" << stStats.un32SentResponseRetrans << "\n";
#endif
	
	return output;
};


