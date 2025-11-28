////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: DtlsContext
//
//  File Name:  DtlsContext.cpp
//
//  Abstract:
//	  This holds the data that is used for signaling DTLS, setting up a DTLS
//    session and other logic related to DTLS
//
////////////////////////////////////////////////////////////////////////////////

#include "DtlsContext.h"
#include "stiAssert.h"
#include "Random.h"
#include "stiBase64.h"

#include <thread>

using namespace vpe;

std::recursive_mutex DtlsContext::m_mutex{};
std::vector<std::shared_ptr<DtlsContext>> DtlsContext::m_generatedContexts;

std::shared_ptr<DtlsContext> DtlsContext::get()
{
	std::lock_guard<std::recursive_mutex> lock(m_mutex);

	std::shared_ptr<DtlsContext> result;
	if (m_generatedContexts.empty())
	{
		result = std::shared_ptr<DtlsContext>(new DtlsContext());
	}
	else
	{
		result = m_generatedContexts.back();
		m_generatedContexts.pop_back();
	}

	init();

	return result;
}

void DtlsContext::init()
{
	std::thread([]()
		{
			const int GeneratedContextCount {3};
			int count = 0;
			do
			{
				std::lock_guard<std::recursive_mutex> lock(m_mutex);
				m_generatedContexts.push_back(std::shared_ptr<DtlsContext>(new DtlsContext()));
				count = m_generatedContexts.size();
			} while (count < GeneratedContextCount);
		}).detach();
}

DtlsContext::DtlsContext() :
	m_certificate(new SelfSignedCert(HashFuncEnum::Sha1))
{
	auto result = 0;
	auto x509 = m_certificate->certificateGet();
	auto privateKey = m_certificate->privateKeyGet ();

	m_sslContext = std::shared_ptr<SSL_CTX>(SSL_CTX_new(DTLS_method()), [](SSL_CTX *p) { SSL_CTX_free(p); });

	/** read_ahead should be set, otherwise DTLS might not work correctly **/
	SSL_CTX_set_read_ahead(m_sslContext.get(), 1);

	/** Tell context that SRTP extension should be used **/
	result = SSL_CTX_set_tlsext_use_srtp(m_sslContext.get(), SupportedProfiles.c_str());
	stiASSERT(result == 0); // For some reason this function returns 0 for success

	/** Ask to check peer certificate and provide peer verification callback `verify`**/
	SSL_CTX_set_verify(m_sslContext.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &DtlsContext::verifyPeerCertificateCB);

	SSL_CTX_set_security_level(m_sslContext.get(), 0);

	result = SSL_CTX_use_cert_and_key(m_sslContext.get(), x509, privateKey, nullptr, 1);
	stiASSERT(result);
}

bool DtlsContext::isNegotiated()
{
	return !m_remoteFingerprint.empty() && m_remoteHashFunction != HashFuncEnum::Undefined;
}

std::shared_ptr<SSL_CTX> DtlsContext::sslContextGet()
{
	return m_sslContext;
}


std::vector<uint8_t> DtlsContext::localFingerprintGet()
{
	return m_certificate->fingerprintGet();
}

RvDtlsSrtpKeyExchangeMode DtlsContext::localKeyExchangeModeGet()
{
	return m_localKeyExchangeMode;
}

void DtlsContext::localKeyExchangeModeSet(RvDtlsSrtpKeyExchangeMode value)
{
	m_localKeyExchangeMode = value;
}

HashFuncEnum vpe::DtlsContext::localFingerprintHashFunctionGet ()
{
	return m_certificate->hashFunctionGet ();
}

std::string DtlsContext::localCNameGet()
{
	if (m_localCName.empty())
	{
		RandomBytesEngine rand;
		m_localCName = base64Encode(rand.bytesGet(16));
	}
	return m_localCName;
}

void DtlsContext::remoteFingerprintSet(const std::vector<uint8_t>& remoteFingerprint)
{
	m_remoteFingerprint = remoteFingerprint;
}

std::vector<uint8_t> DtlsContext::remoteFingerprintGet()
{
	return m_remoteFingerprint;
}

void DtlsContext::remoteFingerprintHashFuncSet(HashFuncEnum value)
{
	m_remoteHashFunction = value;
}

HashFuncEnum DtlsContext::remoteFingerprintHashFuncGet()
{
	return m_remoteHashFunction;
}

RvDtlsSrtpKeyExchangeMode DtlsContext::remoteKeyExchangeModeGet() const
{
	return m_remoteKeyExchangeMode;
}

void DtlsContext::remoteKeyExchangeModeSet(RvDtlsSrtpKeyExchangeMode value)
{
	m_remoteKeyExchangeMode = value;
}

RvDtlsSrtpKeyExchangeMode DtlsContext::setupRoleToKeyExchangeMode (RvSdpConnSetupRole value)
{
	auto result = DTLS_KEY_EXCHANGE_PASSIVE;
	switch (value)
	{
		case RvSdpConnSetupRole::RV_SDPSETUP_ACTIVE:
			result = DTLS_KEY_EXCHANGE_ACTIVE;
			break;

		case RvSdpConnSetupRole::RV_SDPSETUP_PASSIVE:
			result = DTLS_KEY_EXCHANGE_PASSIVE;
			break;

		case RvSdpConnSetupRole::RV_SDPSETUP_ACTPASS:
			result = DTLS_KEY_EXCHANGE_PASSACT;
			break;

		default:
			stiASSERT (false);
			break;
	}

	return result;
}

RvSdpConnSetupRole vpe::DtlsContext::setupRoleFromRemote (RvSdpConnSetupRole value)
{
	switch (value)
	{
		case RV_SDPSETUP_ACTIVE:
		case RV_SDPSETUP_ACTPASS:
			return RV_SDPSETUP_PASSIVE;

		case RV_SDPSETUP_PASSIVE:
			return RV_SDPSETUP_ACTIVE;

		default:
			stiASSERTMSG (false, "Invalid remote setup role: %d\n", value);
			return RV_SDPSETUP_UNKNOWN;
	}
}


int DtlsContext::verifyPeerCertificateCB (int prev, X509_STORE_CTX* ctx)
{
	if (prev == 0)
	{
		//* `prev == 0` means first certificate in the chain

		//* We don't mind that it may be self-signed or even expired, so return `1` in all those cases.
		int err = X509_STORE_CTX_get_error (ctx);
		switch (err)
		{
			case X509_V_OK:
			case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
			case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
			case X509_V_ERR_CERT_HAS_EXPIRED:
				return 1;

			default:
				return 0;
		}
	}
	return 1;
}
