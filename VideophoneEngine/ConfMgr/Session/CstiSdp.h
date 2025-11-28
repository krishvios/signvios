/*!
 * \file CstiSdp.h
 * \brief Wraps a pointer to the RvSdpMsg data structure so that it gets freed
 * when an object of this type goes out of scope.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2016 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include "stiSVX.h"
#include "RvSipStack.h"
#include "rvsdp.h"
#include "SDESKey.h"
#include "stiRtpChannelDefs.h"
#include <memory>
#include <functional>
#include <vector>

class CstiSdp
{
	std::function<void (RvSdpMsg *)> rvSdpDeleter = [](RvSdpMsg *p){rvSdpMsgDestruct (p);};
	using RvSdpMsgType = std::unique_ptr<RvSdpMsg, decltype(rvSdpDeleter)>;

public:

	CstiSdp () = default;

	CstiSdp (
		const CstiSdp &other)
	{
		m_sdpMsg = RvSdpMsgType(rvSdpMsgConstruct (nullptr), rvSdpDeleter);

		rvSdpMsgCopy (m_sdpMsg.get (), other.m_sdpMsg.get ());
	}

	~CstiSdp () = default;

	CstiSdp (CstiSdp &&other) = delete;

	CstiSdp &operator= (
		const CstiSdp &other)
	{
		m_sdpMsg = RvSdpMsgType(rvSdpMsgConstruct (nullptr), rvSdpDeleter);

		rvSdpMsgCopy (m_sdpMsg.get (), other.m_sdpMsg.get ());

		return *this;
	}

	CstiSdp &operator= (CstiSdp &&other) = delete;

	stiHResult initialize (RvSipMsgHandle hMsg);

	bool empty () const
	{
		return m_sdpMsg == nullptr;
	}

	void Print () const;

	int numberOfMediaDescriptorsGet () const;

	const RvSdpMediaDescr *mediaDescriptorGet (int i) const;

	RvSdpMediaDescr *mediaDescriptorGet (int i);

	RvSdpMediaDescr *mediaDescriptorAdd (
		RvSdpMediaType mediaType,
		RvInt32 rtpPort,
		RvSdpProtocol mediaProtocol);

	const RvSdpOrigin *originGet () const;

	const RvSdpConnection *connectionGet () const;

	RvSdpConnection *connectionGet ();

	const RvSdpBandwidth *bandwidthGet () const;

	size_t numberOfAttributesGet () const;

	const RvSdpAttribute *attributeGet (size_t i) const;

	void mediaDescriptorPortSet (int i, int port);

	stiHResult encodeToString (
		std::string &sdpBuffer) const;

	RvSdpMsg *sdpMessageGet () const;

	stiHResult originSet (
		const std::string &username,
		const std::string &sessionId,
		const std::string &version,
		RvSdpNetType netType,
		RvSdpAddrType addrType,
		const std::string &address);

	stiHResult sessionNameSet (
		const std::string &sessionName);

	stiHResult connectionSet (
		RvSdpNetType netType,
		RvSdpAddrType addrType,
		const std::string &address);

	void sessionTimeAdd (
		RvSdpTimeVal start,
		RvSdpTimeVal stop);

	void bandwidthAdd (
		const std::string &bwType,
        int bandwidth);

	void attributeAdd (
		const std::string &name,
		const std::string &value);
	
	bool mediaTypeContainsAttribute (
		const std::string &mediaTypeWanted,
		const std::string &attributeName);

private:

	RvSdpMsgType m_sdpMsg = nullptr;
};
