////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: SelfSignedCert
//
//  File Name:  SelfSignedCert.h
//
//  Abstract:
//	See SelfSignedCert.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <map>
#include <vector>
#include <functional>
#include <cinttypes>
#include <string>

#include <openssl/evp.h>
#include <openssl/x509.h>

#include "stiSVX.h"

namespace vpe
{
	class SelfSignedCert
	{
	public:
		SelfSignedCert(HashFuncEnum hashFunction);
		~SelfSignedCert();

		SelfSignedCert (const SelfSignedCert& other) = delete;
		SelfSignedCert (SelfSignedCert&& other) = delete;
		SelfSignedCert& operator= (const SelfSignedCert& other) = delete;
		SelfSignedCert& operator= (SelfSignedCert&& other) = delete;

		X509* certificateGet();
		EVP_PKEY* privateKeyGet();
		std::vector<uint8_t> fingerprintGet();
		HashFuncEnum hashFunctionGet () { return m_hashFunction; }

		static std::vector<uint8_t> fingerprintGenerate(X509* certificate, HashFuncEnum hashFuncEnum);

	private:
		EVP_PKEY* m_privateKey{};
		X509* m_certificate{};
		std::vector<uint8_t> m_fingerprint;
		HashFuncEnum m_hashFunction;
		static std::map<HashFuncEnum, std::function<const EVP_MD* ()>> m_hashFunctions;

	};
}
