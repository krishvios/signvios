////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiAesEncryptionContext
//
//  File Name:	CstiAesEncryptionContext.h
//
//	Abstract:
//		See CstiAesEncryptionContext.cpp for abstract.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef STIAESUTILS_H
#define STIAESUTILS_H

#include <openssl/aes.h>
#include "rvtypes.h"
#include "rvsrtpaesplugin.h"
#include "rvsrtpsha1.h"
#include "rv_srtp.h"

#include <list>


enum EstiEncryptionDirection {
	estiAESENCRYPTION_DIRECTION_ENCRYPT = 0,
	estiAESENCRYPTION_DIRECTION_DECRYPT = 1
};

enum EstiEncryptionMode {
	estiAESENCRYPTION_MODE_NONE = -1,
	estiAESENCRYPTION_MODE_ECB = 0, // Electronic Codebook Mode
	estiAESENCRYPTION_MODE_CBC = 1  // Cipher Block Chaining Mode 
};

		
ShaStatus Sha1Calculate (
	 RvUint8 *secretKey,
	 RvUint32 keyLength,
	 RvUint8 *inputBuf1,
	 RvUint32 inputBuf1Length,
	 RvUint8 *inputBuf2,
	 RvUint32 inputBuf2Length,
	 RvUint8 *outputBuf,
	 RvUint32 outputBufLength
);

/********************************************************************************
 * type: CstiAesEncryptionContext
 *  description:	
 *          This class is used for encrypting and decrypting buffers
 *          using the AES (Rijndael) algorithm that is implemented in openssl library.
 *          It supports ECB and CBC modes of encryption.
 *
 * NOTE: this has been adapted from the radvision test app code
 *********************************************************************************/
class CstiAesEncryptionContext
{
public:
	CstiAesEncryptionContext () = default;
	~CstiAesEncryptionContext () = default;

	CstiAesEncryptionContext (const CstiAesEncryptionContext &other) = delete;
	CstiAesEncryptionContext (CstiAesEncryptionContext &&other) = delete;
	CstiAesEncryptionContext &operator= (const CstiAesEncryptionContext &other) = delete;
	CstiAesEncryptionContext &operator= (CstiAesEncryptionContext &&other) = delete;

	void Process (
		const RvUint8 *input,
		RvUint8 *output,
		RvUint32 length);

	RvBool ECBModeInit (
		EstiEncryptionDirection direction,
		const RvUint8 *key,
		RvUint16 keyBitSize,
		RvUint16 blockBitSize);
	RvBool CBCModeInit (
		EstiEncryptionDirection direction,
		const RvUint8 *key,
		RvUint16 keyBitSize,
		RvUint16 blockBitSize,
		const RvUint8 *iv);

private:
	RvUint8 m_blockWidth {0}; // Width of a data block in bytes
	EstiEncryptionMode  m_mode {estiAESENCRYPTION_MODE_ECB}; // Encryption mode
	EstiEncryptionDirection m_encryptionDirection {estiAESENCRYPTION_DIRECTION_ENCRYPT}; // control whether we are encrypting or decrypting data
	AES_KEY  m_AesKey {}; // open ssl structure
	RvUint8  m_initializationVector[AES_BLOCK_SIZE]; // open ssl initialization vector
	bool m_initialized {false};
	
	bool AesEncryptionIntitialize (
		const RvUint8 *key,
		RvUint16 keySize,
		RvUint16 blockSize,
		RvUint8 direction);
	
	std::list<const RvUint8 *> m_initalizedKeys{};
};


#endif	// STIAESUTILS_H


