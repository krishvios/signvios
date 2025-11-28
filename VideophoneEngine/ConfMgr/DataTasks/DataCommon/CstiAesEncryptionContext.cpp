////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiAesEncryptionContext
//
//  File Name:	CstiAesEncryptionContext.cpp
//
//	Abstract:
//		A utility class to maintain one single encryption context
//
////////////////////////////////////////////////////////////////////////////////
#include "CstiAesEncryptionContext.h"
#include <openssl/sha.h>
#include "stiSVX.h"


/*!
* ECBModeInit ()
* \brief Initializes the object for ECB m_eMode encryption or decryption.
* \return RV_TRUE is successful, otherwise rvFalse.
*/
RvBool CstiAesEncryptionContext::ECBModeInit (
	EstiEncryptionDirection direction, ///! A flag to set the object to m_eEncryptionDirection or decrypt.
	const RvUint8 *key, ///! The key to use.
	RvUint16 keyBitSize, ///! The size of the key in bits.
	RvUint16 blockBitSize) ///! The size of the block in bits.
{
	RvBool succeeded = RV_TRUE;

	m_mode = estiAESENCRYPTION_MODE_ECB;
	m_encryptionDirection = direction;

	succeeded = AesEncryptionIntitialize (key, keyBitSize, blockBitSize, direction);

	return succeeded;
}


/*!
* CBCModeInit ()
* \brief Initializes the object for CBC m_eMode encryption or decryption.
* \return RV_TRUE is successful, otherwise rvFalse.
*/
RvBool CstiAesEncryptionContext::CBCModeInit (
	EstiEncryptionDirection direction, ///! A flag to set the object to m_eEncryptionDirection or decrypt.
	const RvUint8 *key, ///! The key to use.
	RvUint16 keyBitSize, ///! The size of the key in bits
	RvUint16 blockBitSize, ///! The size of the block in bits
	const RvUint8 *iv) ///! An intitialization vector
{
	RvBool succeeded = RV_TRUE;

	m_mode = estiAESENCRYPTION_MODE_CBC;
	m_encryptionDirection = direction;

	succeeded = AesEncryptionIntitialize (key, keyBitSize, blockBitSize, direction);
	// Store the initialization vector
	memcpy (m_initializationVector, iv, AES_BLOCK_SIZE);

	return succeeded;
}


/*!
* Process ()
* \brief Process the input buffer based on how the object was initialized.
* \return nothing
*/
void CstiAesEncryptionContext::Process (
	const RvUint8 *input, ///! The pInput buffer.
	RvUint8 *output, ///! The pOutput buffer. This can be the same buffer as the pInput buffer.
	RvUint32 length) ///! The size of the buffers in bytes.
{
	int numBlocks  = length / (m_blockWidth * 4);

	if (m_encryptionDirection == estiAESENCRYPTION_DIRECTION_ENCRYPT)
	{
		switch (m_mode)
		{
			case estiAESENCRYPTION_MODE_ECB:
			{
				// NOTE: Open SSL AES_ecb_encrypt works with block 128bit only
				for (int i = 0; i < numBlocks; i++)
				{
					AES_ecb_encrypt (input + i * AES_BLOCK_SIZE, output + i * AES_BLOCK_SIZE, &m_AesKey, (const int)AES_ENCRYPT);
				}
				break;
			}

			case estiAESENCRYPTION_MODE_CBC:
			{
				// NOTE: Open SSL AES_cbc_encrypt works with block 128bit only
				AES_cbc_encrypt (input, output, length, &m_AesKey, m_initializationVector, (const int)AES_ENCRYPT);
				break;
			}

			case estiAESENCRYPTION_MODE_NONE:
			{
				if (output != input)
				{
					memcpy (output, input, length);
				}
				break;
			}
		}
	}
	else
	{
		// Decrypt
		switch (m_mode)
		{
			case estiAESENCRYPTION_MODE_ECB:
			{
				// NOTE: Open SSL AES_ecb_encrypt works with block 128bit only
				for (int i = 0; i < numBlocks; i++)
				{
					AES_ecb_encrypt (input + i * AES_BLOCK_SIZE, output + i * AES_BLOCK_SIZE, &m_AesKey, (const int)AES_DECRYPT);
				}
				break;
			}

			case estiAESENCRYPTION_MODE_CBC:
			{
				// NOTE: Open SSL AES_cbc_encrypt works with block 128bit only
				AES_cbc_encrypt (input, output, length, &m_AesKey, m_initializationVector, (const int)AES_DECRYPT);
				break;
			}

			case estiAESENCRYPTION_MODE_NONE:
			{
				if (output != input)
				{
					memcpy (output, input, length);
				}
				break;
			}
		}

	}
}


/*!
* AesEncryptionIntitialize ()
* \brief Initialize Aes encryption for this context.
* \return true on success
*/
bool CstiAesEncryptionContext::AesEncryptionIntitialize (
	const RvUint8 *key, ///!
	RvUint16 keySize, ///!
	RvUint16 blockSize, ///!
	RvUint8 direction) ///!
{
	bool success = true;
	bool initalized = false;
	
	for (auto &initalizedKey : m_initalizedKeys)
	{
		if (initalizedKey == key)
		{
			initalized = true;
		}
	}
	
	if (!initalized)
	{
		m_initalizedKeys.push_back(key);
		switch (blockSize)
		{
			case 128:
				m_blockWidth = 4;
				break;

			case 192:
				m_blockWidth = 6;
				break;

			case 256:
				m_blockWidth = 8;
				break;

			default:
				success = false;
		}

		if (success)
		{
			if (direction == estiAESENCRYPTION_DIRECTION_ENCRYPT)
			{
				AES_set_encrypt_key (key, keySize, &m_AesKey);
			}
			else
			{
				AES_set_decrypt_key (key, keySize, &m_AesKey);
			}

			if (m_initialized)
			{
				vpe::trace ("already initialized\n");
			}

			m_initialized = true;
		}
	}

	return success;
}


/*!
* truncate ()
* \brief Utility to truncate a data buffer.
* \return ShaStatus
*/
ShaStatus truncate (
	RvChar *inputBuf, ///! data to be truncated
	RvChar *outputBuf, ///! truncated data
	RvUint32 len ///! nLength in bytes to keep
)
{
	RvUint32 cnt;

	if ((NULL == inputBuf) || (NULL == outputBuf))
	{
		return shaNull;
	}

	for (cnt = 0 ; cnt < len; cnt++)
	{
		outputBuf[cnt] = inputBuf[cnt];
	}

	return shaSuccess;
}


/*!
* Sha1Calculate ()
* \brief Perform Sha1 encoding.
*
* General:
*   According to the standard, the message must be padded to an even
*   512 bits. The first padding bit must be a ’1’. The last 64
*   bits represent the nLength of the original message. All bits in
*   between should be 0. This function will pad the message
*   according to those rules by filling the messageBlock array
*   accordingly. It will also call the ProcessMessageBlock function
*   provided appropriately. When it returns, it can be assumed that
*   the message digest has been computed.
*
* \return ShaStatus
*
* NOTE: this is adapted from Radvision's sample application code
* (rtptSha1.c).  Changes:
*  * add error checking
*  * avoid SHA_DIGESTSIZE/SHA_BLOCKSIZE and use openssl defines
*
* NOTE2: this originates from RFC 2202
*/
ShaStatus Sha1Calculate (
	 RvUint8 *secretKey, ///! secret key
	 RvUint32 keyLength, ///! nLength of the key in bytes
	 RvUint8 *inputBuf1, ///! data
	 RvUint32 inputBuf1Len, ///! nLength of data in bytes
	 RvUint8 *inputBuf2, ///! data
	 RvUint32 inputBuf2Len, ///! nLength of data in bytes
	 RvUint8 *outputBuf, ///! output buffer, at least "outputBufLen" bytes
	 RvUint32 outputBufLen ///! in bytes
)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	int result;
	ShaStatus shaStatus;
	SHA_CTX innerContext;
	SHA_CTX outerContext;
	RvUint8 isha[SHA_DIGEST_LENGTH];
	RvUint8 osha[SHA_DIGEST_LENGTH];
	RvUint8 key[SHA_DIGEST_LENGTH];
	RvUint8 buf[SHA_CBLOCK];
	RvUint32 cnt;

	if (keyLength > SHA_CBLOCK)
	{
		SHA_CTX tempContext;

		result = SHA1_Init (&tempContext);
		// NOTE: SHA1_Init,SHA1_Update,SHA1_Final return 1 on success, 0 on error
		stiTESTCOND (1 == result, stiRESULT_ERROR);

		result = SHA1_Update (&tempContext, secretKey, keyLength);
		stiTESTCOND (1 == result, stiRESULT_ERROR);

		result = SHA1_Final (key, &tempContext);
		stiTESTCOND (1 == result, stiRESULT_ERROR);

		secretKey = key;
		keyLength = SHA_DIGEST_LENGTH;
	}

	// Inner Digest
	result = SHA1_Init (&innerContext);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	// Pad the key for inner digest
	for (cnt = 0 ; cnt < keyLength ; ++cnt)
	{
		buf[cnt] = (RvUint8) (secretKey[cnt] ^ 0x36);
	}
	for (cnt = keyLength ; cnt < SHA_CBLOCK ; ++cnt)
	{
		buf[cnt] = 0x36;
	}

	result = SHA1_Update (&innerContext, buf, SHA_CBLOCK);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	result = SHA1_Update (&innerContext, inputBuf1, inputBuf1Len);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	result = SHA1_Update (&innerContext, inputBuf2, inputBuf2Len);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	result = SHA1_Final (isha, &innerContext);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	// Outer Digest
	result = SHA1_Init (&outerContext);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	// Pad the key for outer digest
	for (cnt = 0 ; cnt < keyLength ; ++cnt)
	{
		buf[cnt] = (RvUint8) (secretKey[cnt] ^ 0x5C);
	}
	for (cnt = keyLength ; cnt < SHA_CBLOCK ; ++cnt)
	{
		buf[cnt] = 0x5C;
	}

	result = SHA1_Update (&outerContext, buf, SHA_CBLOCK);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	result = SHA1_Update (&outerContext, isha, SHA_DIGEST_LENGTH);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	result = SHA1_Final (osha, &outerContext);
	stiTESTCOND (1 == result, stiRESULT_ERROR);

	// truncate and prRvUint32 the results
	outputBufLen = ( (outputBufLen > SHA_DIGEST_LENGTH) ? SHA_DIGEST_LENGTH : outputBufLen);
	shaStatus = truncate ( (RvChar *)osha, (RvChar *)outputBuf, outputBufLen);
	stiTESTCOND (shaStatus == shaSuccess, stiRESULT_ERROR);

STI_BAIL:

	return hResult == stiRESULT_SUCCESS ? shaSuccess : shaStateError;
}
