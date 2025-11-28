/*!
 * \file Transaction.h
 * \brief Transaction class
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once
#include "stiSVX.h"
#include "./Message.h"
#include "RvSipTransactionTypes.h"

namespace vpe
{
namespace sip
{
	class Transaction
	{
	public:

		Transaction (RvSipTranscHandle hTransaction);

		Transaction (CstiSipCallLegSP callLeg);

		Message inboundMessageGet() const;

		Message outboundMessageGet() const;

		stiHResult requestSend(const std::string &method);

		std::string methodGet();

		stiHResult sendResponse (uint16_t responseCode);

		operator RvSipTranscHandle ()
		{
			return m_hTransaction;
		}

	private:
		CstiSipCallLegSP m_callLeg;
		RvSipTranscHandle m_hTransaction {nullptr};
	};
}
}
