/*!
 * \file stiSSLTools.cpp
 * \brief This Module contains functions for using OpenSSL libraries
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
 */



//
// Includes
//
#include "stiSSLTools.h"


#include "stiTrace.h"
#include "stiError.h"

#include "stiOS.h"
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <sstream>

#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/SSLManager.h"
#include <Poco/SharedPtr.h>

#include "SslCertificateHandler.h"


using namespace Poco;
using namespace Poco::Net;

//
// Constants
//
const std::map<CertificateType, std::string> certificateFiles =
{
   	{CertificateType::CORE_CA, "CAfile.crt"}, 
	{CertificateType::AMAZON, "amazon-combined.pem"},
	{CertificateType::GODADDY_G2, "gdig2.crt.pem"},
	{CertificateType::GODADDY_G2_CA, "gdroot-g2.crt"},
	{CertificateType::TRUSTEDROOT_DIGICERTCA, "TrustedRoot-DigiCertCA.pem"}
}; 

//
// Typedefs
//
#define MS_CALLBACK

// For a little extra on-chip security break the certificate password in half.
#define SSL_CERT_PASSWORD_FIRST "vAfr"
#define SSL_CERT_PASSWORD_SECOND "2k9t"

//
// Forward Declarations
//

//
// Locals
//
static std::string randFile;
static const char rnd_seed[] = "This is the string to make the random number generator think it has entropy";

//
// Function Definitions
//

std::string certFilenameGet (
	CertificateType certType)
{
	std::stringstream certificateFileName;

	std::string staticDataFolder;

	stiOSStaticDataFolderGet (&staticDataFolder);
	auto certificateFile = certificateFiles.find(certType);
	certificateFileName << staticDataFolder << "certs/" << certificateFile->second;

	return certificateFileName.str();
}


#if 0
/*!
 * \brief Callback used to get the password for unlocking a given certificate.
 *
 * (Required, if it contains a private key.)
 *
 * If this is not used, the app will ask via the command
 * prompt, which is almost certainly a bad choice for a
 * videophone.
 *
 * \return String length of the password.
 */
static int
MS_CALLBACK cert_password_callback (
	char *pszBuffer, //!< Buffer to store the password to
	int nBufferSize, //!< The size of pszBuffer
	int nRwFlag,     //!< Read/write flag
	void *pExtraData //!< Extra data passed to callback (nothing expected)
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("Password callback\n");
	);

	// NOTE: Order here is important.  We're keeping the password
	// halves non-contiguous and backwards.
	const char *szSecondHalf = SSL_CERT_PASSWORD_SECOND;
	int nLength;
	const char *szFirstHalf = SSL_CERT_PASSWORD_FIRST;

	nLength = strlen (szFirstHalf) + strlen (szSecondHalf);
	if (nLength > nBufferSize)
	{
		nLength = nBufferSize;
	}
	snprintf (pszBuffer, nLength + 1, "%s%s", szFirstHalf, szSecondHalf);
	pszBuffer[nLength] = '\0';

	return nLength;
}
#endif


/*!
 * \brief Callback used when verifying a given certificate.
 *
 * Mainly used for determining root causes of errors.
 *
 * \retval 1 pass verification
 * \retval anythin_else fail verification
 * \return The SSL error code.
 */
static int MS_CALLBACK verify_callback (
	int ok,              //!< Indicates whether or not preverfication passed
	X509_STORE_CTX * ctx //!< The SSL context
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("Verify Callback\n");
	);

	char buf[256];
	X509 * cert;
	int err, depth;
	int nRet = 1;	// default no error

	// Get verify results
	cert = X509_STORE_CTX_get_current_cert (ctx);
	err = X509_STORE_CTX_get_error (ctx);
	depth =	X509_STORE_CTX_get_error_depth (ctx);

	// Log verification information
	X509_NAME_oneline (X509_get_subject_name (cert), buf, sizeof (buf));

	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("depth = %d %s\n", depth, buf);
	); // stiDEBUG_TOOL

	// Verification error is found
	if (!ok)
	{
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace ("verify error - num = %d : %s\n", err, X509_verify_cert_error_string (err));
		); // stiDEBUG_TOOL

		switch (err)
		{
		case X509_V_ERR_CERT_NOT_YET_VALID:
			{
				// The certificate is not yet valid: the notBefore date is after the current time
				stiDEBUG_TOOL (g_stiSSLDebug,
					ASN1_TIME * cert_time;
					char * pstring;
					cert_time = X509_get_notBefore (cert);
					pstring = (char*) cert_time->data; // format YYMMDDHHMMSSZ
					stiTrace("notBefore = %s\n", pstring);
				);

				// We would like to continue with the verification process and handshake in this case,
				// because we might not get the current time information from core server yet.
				nRet = 1;
			}
			break;
		case X509_V_ERR_CERT_HAS_EXPIRED:
			{
				// The certificate has expired: that is the notAfter date is before the current time.
				stiDEBUG_TOOL (g_stiSSLDebug,
					ASN1_TIME * cert_time;
					char * pstring;
					cert_time = X509_get_notAfter (cert);
					pstring = (char*) cert_time->data; // format YYMMDDHHMMSSZ
					stiTrace("notAfter = %s\n", pstring);
				);

				// We would like to continue with the verification process and handshake in this case,
				// because we might not have the latest CA root certificate
				nRet = 1;
			}
			break;
		case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
		{
			// HINT: The CAfile MUST include a base64 export *FROM THE SERVER* of the authorizing CA.
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace("The certificate's CA is not on file (%s).\n", certFilenameGet(CertificateType::TRUSTEDROOT_DIGICERTCA).c_str ());
			);
			nRet = 0;
			break;
		}

		default:
			// If verify_callback returns 0, the verification process is immediately stopped
			// with ``verification failed'' state.
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("Other failure reason: %d\n", err);
			);
			nRet = 0;
			break;
		} // end switch
	}
	else
	{
		// If verify_callback returns 1, the verification process is continued.
		nRet = 1;
	}

	return nRet;
}


/*!
 * \brief Seed the random number generator.
 *
 * \return SSL error code.
 */
int stiRANDSeed ()
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSeedPRNG\n");
	); // stiDEBUG_TOOL

	int nRet = 0;
	// Actions to seed PRNG
	RAND_seed(rnd_seed, sizeof(rnd_seed));

	unsigned long seed = stiOSTickGet ();
	RAND_add(&seed, sizeof (seed), 0);
#if APPLICATION != APP_NTOUCH_MAC  // Following causes sandbox violation on Mac
	seed = static_cast<unsigned long>(time(nullptr));
	RAND_add(&seed, sizeof (seed), 0);

	std::string DynamicDataFolder;
	std::stringstream randFilePath;

	stiOSDynamicDataFolderGet (&DynamicDataFolder);

	randFilePath << DynamicDataFolder << ".rnd";
	randFile = randFilePath.str ();

	// Note: normally one would place this file where the function below suggests,
	// (which is $HOME/.rnd) but this is grossly wrong for the embedded device!
	//
	// pRandFileName RAND_file_name (aBuffer, sizeof (aBuffer));
	if (!randFile.empty ())
	{
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace ("RAND_file_name %s\n", randFile.c_str ());
		); // stiDEBUG_TOOL

		if (-1 != RAND_write_file (randFile.c_str ()))
		{
			nRet = RAND_load_file (randFile.c_str (), -1);
		}
		else
		{
			nRet = -1;

			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("RAND_write_file ERROR - bytes written were generated without appropriate seed\n");
			); // stiDEBUG_TOOL
		}
	}
	else
	{
		nRet = -1;

		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace ("RAND_file_name ERROR\n");
		); // stiDEBUG_TOOL
	}
#endif
	return nRet;
}


/*!
 * \brief Saves the last randomization to use next time.
 *
 * \return void
 */
void stiRANDCleanup()
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiRANDCleanup\n");
	); // stiDEBUG_TOOL

	if (!randFile.empty ())
	{
		RAND_cleanup ();

		/* One way to get more entopy is to keep our file
		char command[256];
		sprintf(command, "rm -f %s", randFile.c_str ());

		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("system call: %s\n",command);
		);

		int nSysStatus = system (command);
		nSysStatus = WEXITSTATUS (nSysStatus);
		if (0 != nSysStatus)
		{
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace("system call: %s ERROR %d\n", nSysStatus);
			);
		}
		*/

		randFile.clear ();
	}
}

/*!
 * \brief Creates a new SSL context.
 *
 * This also sets up the context the way
 * we want it.
 *
 * \return pointer to the new context.
 */
#define ERR_MSG_SIZE 255
SSL_CTX * stiSSLCtxNew ()
{
	SSL_CTX *ctx = nullptr;
	stiHResult hResult = stiRESULT_SUCCESS;
	int err = 1;
	std::string errorString;
	std::string errorFilename;
	const SSL_METHOD *pMethod = nullptr;

	// action to seed PRNG
	if (-1 == stiRANDSeed ())
	{
		errorString = "random seed fail";
		stiTESTCOND (false, stiRESULT_ERROR);
	}
	
	// Get TLS v1.2 method
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	pMethod = TLSv1_2_client_method ();
#else /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	pMethod = TLS_client_method ();
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	
	ctx = SSL_CTX_new (pMethod);
	if (nullptr == ctx)
	{
		errorString = "error creating context";
		stiTESTCOND (false, stiRESULT_ERROR);
	}

	for (auto certificate : certificateFiles)
	{
		std::string certificateFile = certFilenameGet(certificate.first);
		// Set trusted CA certificates
		err = SSL_CTX_load_verify_locations(ctx, certificateFile.c_str(), nullptr);
		if (err != 1)
		{
			errorString = "failed to set verify locations";
			errorFilename = certificateFile;
			ERR_print_errors_fp(stderr);
			stiTESTCOND(false, stiRESULT_ERROR);
		}
	}

	// Set the verification flags for ctx to be mode and specify
	// the verify_callback function to be used.
	// The mode has been set to SSL_VERIFY_PEER so that if the verification process fails,
	// the TLS/SSL handshake is immediately terminated with an alert message containing
	// the reason for the verification failure.
	SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, verify_callback);
	SSL_CTX_set_options (ctx, SSL_OP_ALL | SSL_OP_NO_SSLv3);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		const char *pszSslErrMessage = "";

		long nErrcode = err;//ERR_get_error();
		if (nErrcode == 0)
		{
			nErrcode = errno;
			pszSslErrMessage = strerror (nErrcode);
		}
		else
		{
			static char szSslErrMessage[ERR_MSG_SIZE + 1];
			ERR_error_string_n (nErrcode, szSslErrMessage, ERR_MSG_SIZE);
			pszSslErrMessage = szSslErrMessage;
		}
		// HINT: For a list of SSL error codes, see:
		//		http://www.openssl.org/docs/apps/verify.html#DIAGNOSTICS
		//stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace ("ERR stiSSLCtxNew: %s\n\tSSLError(%d): %s%s%s\n\tctx: %p\n", errorString.c_str(),
				nErrcode, pszSslErrMessage,
				!errorFilename.empty() ? "\n\tFile: " : "", !errorFilename.empty() ? errorFilename.c_str() : "",
				ctx);
		//);
	}

	return ctx;
}


/*!
 * \brief Destroys an SSL context.
 *
 * Must be called for all created contexts
 * before or during app shutdown.
 */
void stiSSLCtxDestroy (
	SSL_CTX * ctx //!< The SSL context to destroy
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSSLCtxDestroy\n");
	); // stiDEBUG_TOOL

	stiRANDCleanup ();

	if (nullptr != ctx)
	{
		SSL_CTX_free (ctx);
		ctx = nullptr;
	}
}


/*!
 * \brief Determine if an ssl session is reused.
 *
 * \return Resued status.
 */
int stiSSLSessionReused (
	SSL * ssl //!< The SSL connection to examine
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSSLSessionReused\n");
	); // stiDEBUG_TOOL

	int bReused = 0;
	if (nullptr != ssl)
	{
		bReused = SSL_session_reused (ssl);
	}

	return bReused;
}


/*!
 * \brief Gets an existing ssl session.
 *
 * \retval NULL unable to get session.
 * \return A pointer to the ssl session.
 */
SSL_SESSION * stiSSLSessionGet (
	SSL * ssl //!< The SSL connection to get a session of
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSSLSessionGet\n");
	); // stiDEBUG_TOOL

	SSL_SESSION * session = nullptr;;
	if (nullptr != ssl)
	{
		session = SSL_get1_session (ssl);
	}

	return session;
}


/*!
 * \brief Terminate an ssl session.
 */
void stiSSLSessionFree (
	SSL_SESSION * session //!<The SSL session to release
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSSLSessionFree\n");
	); // stiDEBUG_TOOL

	if (nullptr != session)
	{
		SSL_SESSION_free (session);
		session = nullptr;
	}
}


/*!
 * \brief Start an ssl session
 *
 * \return A posinter to the ssl object.
 */
SSL * stiSSLConnectionStart (
	SSL_CTX * ctx,					//!< The SSL context
	int nSocket,                    //!< The socket to use
	SSL_SESSION * sessionResumption //!< The resumed session (if any)
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSSLConnectionStart\n");
	); // stiDEBUG_TOOL

	SSL * ssl = SSL_new (ctx);
	if (nullptr != ssl)
	{
		// Connect the SSL object with the socket
		SSL_set_fd (ssl, nSocket);

		if (sessionResumption)
		{
			// resume the session
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("The SSL session resuming ...\n");
			); // stiDEBUG_TOOL

			SSL_set_session (ssl, sessionResumption);
		}

		// Start SSL handshake protocol
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace ("The SSL handshake starting ...\n");
		); // stiDEBUG_TOOL
		int err = SSL_connect (ssl);
		if (err == 1)
		{
/*#ifdef stiDEBUG_OUTPUT //...
			DebugTools::instanceGet()->DebugOutput("vpEngine::stiSSLTools::stiSSLConnectionStart() - Handshake Successful, SSL Connection Established");
#endif*/
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("The SSL handshake was successfully completed, a SSL connection has been established.\n");
			); // stiDEBUG_TOOL

			// Following two steps are optional and not required for
			// data exchange to be successful.

			// Get the cipher - opt
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("SSL connection using %s\n", SSL_get_cipher (ssl));
			);

			// Get and check server's certificate (TYY: for debugging purpose only)
			// SSLCheckCert(ssl);
		}
		else
		{
/*#ifdef stiDEBUG_OUTPUT //...
			const char* vsError[] =
			{
				"SSL_ERROR_NONE",
				"SSL_ERROR_SSL",
				"SSL_ERROR_WANT_READ",
				"SSL_ERROR_WANT_WRITE",
				"SSL_ERROR_WANT_X509_LOOKUP",
				"SSL_ERROR_SYSCALL",
				"SSL_ERROR_ZERO_RETURN",
				"SSL_ERROR_WANT_CONNECT",
				"SSL_ERROR_WANT_ACCEPT"
			}; // SSL_ERROR
			static char msg[256];
			sprintf(msg, "vpEngine::stiSSLTools::stiSSLConnectionStart() - SSL_connect() %s - FAIL *****ERROR*****  *****ERROR*****  *****ERROR*****  *****ERROR*****", vsError[SSL_get_error (ssl, err)]);
			DebugTools::instanceGet()->DebugOutput(msg);
#endif*/
			stiDEBUG_TOOL (g_stiSSLDebug,
				stiTrace ("The SSL handshake was NOT successfully completed - error = %d\n", SSL_get_error (ssl, err));
			); // stiDEBUG_TOOL

			SSL_free (ssl);
			ssl = nullptr;
		}
	}

	return ssl;
}


/*!
 * \brief Free an ssl connection.
 *
 */
void stiSSLConnectionFree (
	SSL * ssl //!< The ssl connection to release
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSSLConnectionFree\n");
	); // stiDEBUG_TOOL

	SSL_free (ssl);
	ssl = nullptr;
}


/*!
 * \brief Shut down an ssl connection.
 */
void stiSSLConnectionShutdown (
	SSL * ssl //!< The ssl connection to shutdown
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace ("stiSSLConnectionShutdown\n");
	); // stiDEBUG_TOOL


	ERR_free_strings();

	// Normally a proper SSL shutdown requires we call SSL_shutdown a second time if SSL_shutdown returns 0, to wait
	// for the other endpoint's close_notify alert. However, many HTTPS servers don't ever respond with these messages.
	// The peer's close_notify response is normally needed to prevent a truncation attack, but because HTTP is explicit
	// about how much data is going to be sent, we know the connection should be closed. So for HTTPS it is standard
	// practice to not wait for the peer's close_notify alert.
	//
	// This fixes a bug on iOS Cricket LTE connections, where the connection doesn't appear to be reset by the peer so
	// we wait for the full timeout (10 seconds) for the peer's close_notify alert on the second SSL_shutdown. This caused
	// logins to take a few minutes.
	SSL_shutdown (ssl);
}


/*!
 * \brief Return the result of a verify operation.
 *
 * \return true if the certificate was verified. Otherwise, false.
 */
bool stiSSLVerifyResultGet (
	SSL * ssl //!< The ssl connection from which we want the verify result
)
{
	int result = X509_V_OK;
	bool verified = false;

	if (nullptr != ssl)
	{
		result = SSL_get_verify_result (ssl);

		verified = (result == X509_V_OK);
	}

	stiDEBUG_TOOL (g_stiSSLDebug,
		if (verified)
		{
			stiTrace ("Certificate Verify OK !!!\n");
		}
		else
		{
			stiTrace ("Certificate Verify Failed - R(%d): %s\n", result, X509_verify_cert_error_string (result));
		}
	); // stiDEBUG_TOOL

	return verified;
}


/*!
 * \brief Begin checking a given certificate for validity.
 *
 */
void SSLCheckCert (
	SSL * ssl //!< The ssl connection whose certificate(s) need verifying
)
{
	if (X509_V_OK != SSL_get_verify_result (ssl))
	{
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace ("certificate doesn't verify\n");
		); // stiDEBUG_TOOL
	}

	X509 * server_cert = SSL_get_peer_certificate (ssl);
	if (nullptr != server_cert)
	{

		stiDEBUG_TOOL (g_stiSSLDebug,
			char buf[256];
			stiTrace ("Server certificate:\n");

			X509_NAME_oneline (X509_get_subject_name (server_cert), buf, 256);
			stiTrace ("\t subject: %s\n", buf);

			X509_NAME_oneline (X509_get_issuer_name  (server_cert), buf, 256);
			stiTrace ("\t issuer: %s\n", buf);

			X509_NAME_get_text_by_NID (X509_get_subject_name(server_cert), NID_commonName, buf, 256);
			stiTrace("peer_CN = %s\n", buf);
		);

		X509_free (server_cert);
	}
}


/*!
* \brief start the SSL_write
* \retval >= 0 for success otherwise for failure
*/
int stiSSLWrite (
	SSL * ssl,   //!< The SSL connection to write to
	const char * pBuf, //!< The data to write
	int nSize    //!< The size of pBuf
)
{
	int err = -1;
	if (nullptr != ssl)
	{
		err = SSL_write (ssl, pBuf, nSize);
	}
	return err;
}


/*!
* \brief start the SSL_read, and decide if a record has been read completely.
* \retval > 0 continue the SSL_read, retval == 0 completely read a record, retval < 0 error
*/
int stiSSLRead (
	SSL * ssl,   //!< The SSL connection to read from
	char * pBuf, //!< The buffer to read the data into
	int nSize    //!< The size of pBuf
)
{
	stiDEBUG_TOOL (g_stiSSLDebug,
		stiTrace("stiSSLRead - ");
	);

	bool bContinue = true;
	bool bReadBlocked = false;
	int nBytesRead = SSL_read(ssl, pBuf, nSize);
	switch(SSL_get_error(ssl, nBytesRead))
	{
	case SSL_ERROR_NONE:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_NONE\n");
		);
		break;
	case SSL_ERROR_ZERO_RETURN:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_ZERO_RETURN\n");
		);
		bContinue = false;
		break;
	case SSL_ERROR_WANT_READ:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_WANT_READ\n");
		);
		bReadBlocked = true;
		break;
	case SSL_ERROR_WANT_WRITE:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_WANT_WRITE\n");
		);
		break;
	case SSL_ERROR_WANT_CONNECT:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_WANT_CONNECT\n");
		);
		break;
	case SSL_ERROR_WANT_X509_LOOKUP:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_WANT_X509_LOOKUP\n");
		);

		break;
	case SSL_ERROR_SYSCALL:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_SYSCALL\n");
		);
		bContinue = false;
		break;
	case SSL_ERROR_SSL:
		stiDEBUG_TOOL (g_stiSSLDebug,
			stiTrace("SSL_ERROR_SLL\n");
		);
		break;
	}

	int err = nBytesRead;  // SSL_read completed
	if (!bContinue)
	{
		err = -1;
	}
	else if (bReadBlocked)
	{
		err = 0;
	}

	return err;
}


/*!
 * \brief Get the expiration date of the current client certificate.
 * certificateType: Which certificate we are to get the expiration date of
 * \return void
 */
std::string stiSSLSecurityExpirationDateGet (CertificateType certificateType)
{
	auto CertFile = certFilenameGet (certificateType);
	std::stringstream expirationDate{};
	// Get a copy of the client cert expiration date
	FILE *pFile = fopen (CertFile.c_str (), "r");
	if (nullptr != pFile)
	{
		X509 *pX509 = nullptr;
		if (nullptr != PEM_read_X509 (pFile, &pX509, nullptr, nullptr))
		{
			auto pszExpirationDate = X509_get_notAfter (pX509)->data;
			expirationDate << pszExpirationDate[2] << pszExpirationDate[3] << "/" // month
				<< pszExpirationDate[4] << pszExpirationDate[5] << "/" // day
				<< "20" << pszExpirationDate[0] << pszExpirationDate[1] << " " // year
				<< pszExpirationDate[6] << pszExpirationDate[7] << ":" // hour
				<< pszExpirationDate[8] << pszExpirationDate[9] << ":" // minute
				<< pszExpirationDate[10] << pszExpirationDate[11] << " GMT"; // second
			X509_free(pX509);
		}
		fclose (pFile);
		pFile = nullptr;
	}
	return expirationDate.str();
}

void pocoSSLCertificatesInitialize()
{
	try
	{
		Poco::Net::initializeSSL();
		//Initializing the SSLManager Once with the AWS Certificate.
		SharedPtr<SslCertificateHandler> certificateHandler = new SslCertificateHandler(false);
		SharedPtr<KeyConsoleHandler> privateKeyHandler = new KeyConsoleHandler(false);
		Context::Ptr context = new Context(Context::CLIENT_USE, "", Context::VERIFY_RELAXED, 9, true, vpe::https::CERT_PARAMS);
		for (auto certificate : certificateFiles)
		{
			try
			{
				std::string certificateFile = certFilenameGet(certificate.first);
				// Set trusted CA certificates
				X509Certificate certificatex509(certificateFile);
				auto expirationDate = certificatex509.expiresOn();
				stiTrace("SSL Certificate %s Expires: %d/%d/%d", certificate.second.c_str(), expirationDate.month(), expirationDate.day(), expirationDate.year());
				context->addCertificateAuthority(certificatex509);
			}
			catch (Exception& ex)
			{
				stiASSERTMSG(false, "%s", ex.displayText().c_str());
			}
		}
		SSLManager::instance().initializeClient(privateKeyHandler, certificateHandler, context);
	}
	catch (Exception& ex)
	{
		stiASSERTMSG(false, "%s", ex.displayText().c_str());
	}
}

// end file stiSSLTools.cpp
