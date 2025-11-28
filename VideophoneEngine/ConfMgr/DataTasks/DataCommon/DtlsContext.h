////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: DtlsContext
//
//  File Name:  DtlsContext.h
//
//  Abstract:
//	See DtlsContext.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "SelfSignedCert.h"

#include "rvdtlske.h"
#include "rvsdpenums.h"

#include <string>
#include <vector>
#include <mutex>
#include <openssl/ssl.h>

namespace vpe
{
	class DtlsContext
	{
	public:
		bool isNegotiated();

		std::shared_ptr<SSL_CTX> sslContextGet();

		std::vector<uint8_t> localFingerprintGet();
		RvDtlsSrtpKeyExchangeMode localKeyExchangeModeGet();
		void localKeyExchangeModeSet(RvDtlsSrtpKeyExchangeMode value);
		HashFuncEnum localFingerprintHashFunctionGet ();
		std::string localCNameGet();

		void remoteFingerprintSet(const std::vector<uint8_t>& remoteFingerprint);
		std::vector<uint8_t> remoteFingerprintGet();

		void remoteFingerprintHashFuncSet(HashFuncEnum value);
		HashFuncEnum remoteFingerprintHashFuncGet();

		RvDtlsSrtpKeyExchangeMode remoteKeyExchangeModeGet() const;
		void remoteKeyExchangeModeSet(RvDtlsSrtpKeyExchangeMode value);

		static RvDtlsSrtpKeyExchangeMode setupRoleToKeyExchangeMode(RvSdpConnSetupRole value);
		static RvSdpConnSetupRole setupRoleFromRemote (RvSdpConnSetupRole value);
		static std::shared_ptr<DtlsContext> get();
		static void init();

	private:
		DtlsContext();

		static int verifyPeerCertificateCB(int prev, X509_STORE_CTX* ctx);
		static std::vector<std::shared_ptr<DtlsContext>> m_generatedContexts;

		// TODO: Change supported profiles to "SRTP_AEAD_AES_256_GCM:SRTP_AEAD_AES_128_GCM:SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32"
		//		 once Radvision supports 256bit DTLS
		const std::string SupportedProfiles = "SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32";

		std::shared_ptr<SelfSignedCert> m_certificate;
		std::shared_ptr<SSL_CTX> m_sslContext{};
		std::string m_localCName;
		std::vector<uint8_t> m_remoteFingerprint;
		RvDtlsSrtpKeyExchangeMode m_localKeyExchangeMode{};
		RvDtlsSrtpKeyExchangeMode m_remoteKeyExchangeMode{};
		HashFuncEnum m_remoteHashFunction{};
		static std::recursive_mutex m_mutex;
	};
}
