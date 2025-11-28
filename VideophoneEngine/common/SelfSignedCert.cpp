////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name: SelfSignedCert
//
//  File Name:  SelfSignedCert.cpp
//
//  Abstract:
//	This class will generate a private key and self signed certificate. It will
//  also generate fingerprints for validating a remote certificate.
//
////////////////////////////////////////////////////////////////////////////////
#include "SelfSignedCert.h"
#include "stiAssert.h"

using namespace vpe;

std::map<HashFuncEnum, std::function<const EVP_MD* ()>> SelfSignedCert::m_hashFunctions =
{
	{HashFuncEnum::Sha1, EVP_sha1},
	{HashFuncEnum::Sha224, EVP_sha224},
	{HashFuncEnum::Sha256, EVP_sha256},
	{HashFuncEnum::Sha384, EVP_sha384},
	{HashFuncEnum::Sha512, EVP_sha512},
	{HashFuncEnum::Md5, EVP_md5},
};

SelfSignedCert::SelfSignedCert (HashFuncEnum hashFunction) :
	m_hashFunction(hashFunction)
{
	//
	// Generate the private key (this can take up to 1 second to complete)
	//
	m_privateKey = EVP_PKEY_new();

	constexpr auto KeySize = 2048;

	// rsa is freed when m_privateKey is freed.
	RSA* rsa = RSA_new ();
	BIGNUM* bignum = BN_new ();
	auto result = BN_set_word (bignum, RSA_F4);
	stiASSERT (result);

	result = RSA_generate_key_ex (rsa, KeySize, bignum, nullptr);
	stiASSERT (result);

	if (bignum != nullptr)
	{
		BN_free (bignum);
		bignum = nullptr;
	}

	EVP_PKEY_assign_RSA(m_privateKey, rsa);

	//
	// Generate a self signed certificate
	//
	m_certificate = X509_new();

	constexpr auto daysFactor = 60 * 60 * 24;

	X509_gmtime_adj(X509_get_notBefore(m_certificate), daysFactor * -1L); // yesterday
	X509_gmtime_adj(X509_get_notAfter(m_certificate), daysFactor * 365L); // a year from now

	X509_set_pubkey(m_certificate, m_privateKey);

	X509_NAME* name;
	name = X509_get_subject_name(m_certificate);

	X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("US"), -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("Sorenson Communications"), -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);

	X509_set_issuer_name(m_certificate, name);

	result = X509_sign(m_certificate, m_privateKey, EVP_sha1());
	stiASSERT(result);

	//
	// Generate the fingerprint
	//
	m_fingerprint = fingerprintGenerate(m_certificate, m_hashFunction);
}

SelfSignedCert::~SelfSignedCert()
{
	if (m_privateKey != nullptr)
	{
		EVP_PKEY_free(m_privateKey);
		m_privateKey = nullptr;
	}

	if (m_certificate != nullptr)
	{
		X509_free(m_certificate);
		m_certificate = nullptr;
	}
}

X509* SelfSignedCert::certificateGet()
{
	return m_certificate;
}

EVP_PKEY* SelfSignedCert::privateKeyGet()
{
	return m_privateKey;
}

std::vector<uint8_t> SelfSignedCert::fingerprintGet()
{
	return m_fingerprint;
}

std::vector<uint8_t> SelfSignedCert::fingerprintGenerate(X509* certificate, HashFuncEnum hashFuncEnum)
{
	unsigned int size = EVP_MAX_MD_SIZE;
	std::vector<uint8_t> fingerprint(size);

	auto digest = m_hashFunctions[hashFuncEnum]();

	auto ret = X509_digest(certificate, digest, fingerprint.data(), &size);
	if (ret)
	{
		fingerprint.resize(size);
		return fingerprint;
	}

	return {};
}

