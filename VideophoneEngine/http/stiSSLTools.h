/*!
 * \file stiSSLTools.h
 * \brief See stiSSLTools.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

//
// Includes
//
#include <openssl/ssl.h>
#include "stiDefs.h"
#include <map>
#include <string>

//
// Constants
//

//
// Typedefs
//
#define SSL_TIME_START 0
#define SSL_TIME_STOP 1

// These are the different types of security certificates we may have installed.
enum class CertificateType
{
	CORE_CA,
	AMAZON,
	GODADDY_G2,
	GODADDY_G2_CA,
	TRUSTEDROOT_DIGICERTCA	
};

//
// Forward Declarations
//

//
// Globals
//
namespace vpe
{
	namespace https
	{
		static constexpr const char* CERT_PARAMS = "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH";
		static constexpr const char* AWS_APIKEY_HEADER = "X-api-key";
		static constexpr const char* JSON_CONTENT_TYPE = "application/json";
	}
}

//
// Function Declarations
//

bool stiSSLVerifyResultGet (SSL * ssl);

SSL_CTX * stiSSLCtxNew ();
void stiSSLCtxDestroy(SSL_CTX * ctx);
SSL * stiSSLConnectionStart (SSL_CTX * ctx, int nSocket, SSL_SESSION * sessionResumption);
void stiSSLConnectionFree (SSL * ssl);
void stiSSLConnectionShutdown (SSL * ssl);
void SSLCheckCert (SSL * ssl);
int stiSSLWrite (SSL * ssl, const char * pBuf, int nSize);
int stiSSLRead (SSL * ssl, char *pBuf, int nSize);
void stiSSLSessionFree (SSL_SESSION * session);
int stiSSLSessionReused (SSL * ssl);
std::string stiSSLSecurityExpirationDateGet (CertificateType certificateType);
SSL_SESSION * stiSSLSessionGet (SSL * ssl);
void pocoSSLCertificatesInitialize();
