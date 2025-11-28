/*!
 * \file Transaction.h
 * \brief Transaction class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
 */
#include "./Transaction.h"
#include "CstiCall.h"
#include "CstiSipCall.h"
#include "CstiSipCallLeg.h"
#include "./Message.h"

namespace vpe
{
namespace sip
{
	Transaction::Transaction (RvSipTranscHandle hTransaction)
	:
		m_hTransaction(hTransaction)
	{
	}

	Transaction::Transaction(CstiSipCallLegSP callLeg)
	:
		m_callLeg(callLeg)
	{
		auto rvStatus = RvSipCallLegTranscCreate (callLeg->m_hCallLeg, nullptr, &m_hTransaction);
		stiASSERT (rvStatus == RV_OK);
	}

	Message Transaction::inboundMessageGet() const
	{
		return 	Message(m_hTransaction, m_callLeg, false);
	}

	Message Transaction::outboundMessageGet() const
	{
		return 	Message(m_hTransaction, m_callLeg, true);
	}

	stiHResult Transaction::requestSend(const std::string &method)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		auto rvStatus = RvSipCallLegTranscRequest (m_callLeg->m_hCallLeg, const_cast<RvChar*>(method.c_str()), &m_hTransaction);
		stiTESTRVSTATUS ();

	STI_BAIL:

		return hResult;
	}

	std::string Transaction::methodGet()
	{
		std::vector<RvChar> buffer;

		buffer.resize(MAX_METHOD_LEN);
		auto rvStatus = RvSipTransactionGetMethodStr (m_hTransaction, buffer.size(), buffer.data());

		if (rvStatus == RV_OK)
		{
			std::string method{buffer.begin(), std::find(buffer.begin(), buffer.end(), '\0')};

			return method;
		}

		return {};
	}

	stiHResult Transaction::sendResponse (uint16_t responseCode)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		auto rvStatus = RvSipTransactionRespond (m_hTransaction, responseCode, nullptr);
		stiTESTRVSTATUS ();

	STI_BAIL:

		return hResult;
	}
}
}
