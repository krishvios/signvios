#include "FileEncryption.h"
#include "stiError.h"

#include <sstream>
#include <openssl/evp.h>

const unsigned char SSL_KEY[] = { 8, 0, 1, 2, 8, 7, 9, 4, 0, 0, 4, 1, 9, 2, 4, 1 };
const unsigned char SSL_IV[] = { 4, 1, 9, 2, 4, 1, 9, 2 };

FileEncryption::FileEncryption (std::string fileName) :
	m_fileName(fileName)
{
}

bool FileEncryption::encrypt ()
{
	bool encrypted = false;
	FILE* decryptedFile = fopen (m_fileName.c_str (), "r");
	if (decryptedFile)
	{
		std::stringstream encFileName;
		encFileName << m_fileName << ".bin";

		FILE* encryptedFile = fopen (encFileName.str ().c_str (), "wb");
		if (encryptedFile)
		{
			if (encryptOrDecrypt (decryptedFile, encryptedFile, true))
			{
				encrypted = true;
			}
			fclose (encryptedFile);
		}
		fclose (decryptedFile);
	}
	return encrypted;
}

std::string FileEncryption::decrypt ()
{
	std::string decryptedFileName;

	std::stringstream encFileName;
	encFileName << m_fileName << ".bin";
	FILE* encFile = fopen (encFileName.str ().c_str (), "rb");
	if (encFile)
	{
		std::stringstream decFileName;
		decFileName << m_fileName << ".tmp";
		FILE* decFile = fopen (decFileName.str ().c_str (), "w");
		if (decFile)
		{
			if (encryptOrDecrypt (encFile, decFile, false))
			{
				decryptedFileName = decFileName.str ();
			}
			fclose (decFile);
		}
		fclose (encFile);
	}
	return decryptedFileName;
}

bool FileEncryption::encryptOrDecrypt (FILE* in, FILE* out, bool do_encrypt)
{
	/* Allow enough space in output buffer for additional block */
	unsigned char inbuf[8096], outbuf[8096 + EVP_MAX_BLOCK_LENGTH];
	int inlen, outlen;
	auto hResult = stiRESULT_SUCCESS;

	auto ctx = EVP_CIPHER_CTX_new ();
	EVP_CipherInit (ctx, EVP_bf_cbc (), SSL_KEY, SSL_IV, do_encrypt);
	for (;;)
	{
		inlen = fread (inbuf, 1, 8096, in);
		if (inlen <= 0)
		{
			break;
		}
		if (!EVP_CipherUpdate (ctx, outbuf, &outlen, inbuf, inlen))
		{
			/* Error */
			stiTHROW (stiRESULT_ERROR);
		}
		fwrite (outbuf, 1, outlen, out);
	}
	if (!EVP_CipherFinal_ex (ctx, outbuf, &outlen))
	{
		/* Error */
		stiTHROW (stiRESULT_ERROR);
	}
	fwrite (outbuf, 1, outlen, out);

STI_BAIL:
	EVP_CIPHER_CTX_free (ctx);
	return !stiIS_ERROR(hResult);
}
