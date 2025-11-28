#pragma once
#include "stiSVX.h"
#include "./ContactHeader.h"
#include "RvSipMsg.h"
#include "RvSipTransactionTypes.h"
#include "RvSipRegClientTypes.h"
#include <tuple>
#include <functional>

class CstiSdp;

namespace vpe
{
namespace sip
{
	class Message
	{
	public:

		enum class Method
		{
			Undefined,
			Invite,
			Ack,
			Bye,
			Register,
			Refer,
			Notify,
			Message,
			Prack,
			Cancel,
			Subscribe,
			Update,
		};

		Message(RvSipMsgHandle hMessage);

		Message(CstiSipCallLegSP callLeg, bool outbound);

		Message(RvSipTranscHandle hTransaction, CstiSipCallLegSP callLeg, bool outbound);

		Message(CstiSipCallLegSP callLeg, RvSipMsgHandle hMessage);

		Message(RvSipRegClientHandle hRegClient, bool outbound);

		void clear ();

		stiHResult headerAdd(const std::string &headerName, const std::string &headerValue);

		stiHResult inviteRequirementsAdd ();

		stiHResult allowIncomingCallHeaderAdd();

		stiHResult convertSipToH323HeaderAdd();

		stiHResult geolocationAdd();

		stiHResult additionalHeadersAdd();

		stiHResult sinfoHeaderAdd(const std::string &systemInfo);

		stiHResult bodySet (const std::string &contentType, const std::string &body);

		stiHResult bodySet (const CstiSdp &Sdp);

		stiHResult bodyClear ();

		Method methodGet () const;

		int statusCodeGet () const;

		EstiTransport viaTransportGet () const;

		std::tuple<std::string, bool> headerValueGet(const std::string &headerName) const;

		RvSipPartyHeaderHandle toHeaderGet();

		std::tuple<std::string, std::string, int, bool> userAgentGet () const;

		std::vector<ContactHeader> contactHeadersGet() const;

		void contactHeadersDelete();

		std::string contentTypeHeaderGet() const;

		std::tuple<std::vector<Message::Method>, bool> allowHeaderGet() const;

		operator bool () const
		{
			return m_message != nullptr;
		}

		operator RvSipMsgHandle () const
		{
			return m_message;
		}

	private:

		void forEachHeaderType (
			RvSipHeaderType headerType,
			std::function<void(void *, RvSipHeaderListElemHandle)> callback) const;

		template<typename T>
		using messageStringFunc = RvStatus (*)(T, RvChar *, RvUint, RvUint *);

		template<typename T>
		Message::Method translateMethodType(RvSipMethodType rvMethodType, T handle, messageStringFunc<T>  func) const;

		template<typename T>
		std::string rvMessageStringGet(T handle, messageStringFunc<T> func) const;

		CstiSipCallLegSP m_callLeg;
		RvSipMsgHandle m_message {nullptr};
	};
}
}
