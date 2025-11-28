////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	stiClientValidate
//
//	File Name:	stiClientValidate.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		Handling of Client Key validation and creation.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//
#include <cstring>
#include "stiClientValidate.h"
#include "stiBase64.h"
#include "stiTrace.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Locals
//

//
// Function Declarations
//

//Android has problems trying to use ifaddrs so we need to use the old method.
#if APPLICATION != APP_NTOUCH_ANDROID
////////////////////////////////////////////////////////////////////////////////
//; stiClientValidate
//
// Description: Validates a client connection key
//
// Abstract:
//
// Returns:
//	estiOK if validation is successful, otherwise estiERROR
//
EstiResult stiClientValidate (
	int nAddressFamily, ///! The family of the supplied addresses (AF_INET or AF_INET6)
	const uint32_t *pun32ClientIP, ///! Pointer to the value of the client address
	const uint32_t *pun32ServerIP, ///! Pointer to the value of the server address
	const char *pszValidationKey) ///! The key we are validating
{
	EstiResult eResult = estiOK;
	
	char szCheckKey[MAX_KEY];
	
	// Create a client key based on the IP Addresses passed in
	stiKeyCreate (nAddressFamily, pun32ClientIP, pun32ServerIP, szCheckKey);

	// Compare the key generated with the key passed in
	if (0 != strcmp (pszValidationKey, szCheckKey))
	{
		eResult = estiERROR;
	} // end if

	return (eResult);
} // end stiClientValidate


////////////////////////////////////////////////////////////////////////////////
//; stiKeyCreate
//
// Description: Creates a client connection key based on two IP addresses
//
// Abstract:
//	Creates a Base64 encoded 128 bit pseudo random number based upon the 
//	client and server IP addresses.
//
// Returns:
//	None
//
void stiKeyCreate (
	int nAddressFamily, ///! The family of the supplied addresses (AF_INET or AF_INET6)
	const uint32_t *pun32ClientIP, ///! Pointer to the value of the client address
	const uint32_t *pun32ServerIP, ///! Pointer to the value of the server address
	char *pszKey)  ///! Buffer for the generated key
{
	uint32_t aun32ValidationKey[4];
	unsigned int x;
	int nIpLength = (nAddressFamily == AF_INET6) ? 4 : 1;

	// Initialize the first location of the key with a transform of the 
	// Client IP
	aun32ValidationKey[0] = 0x532fa3db;
	for (int i = 0; i < nIpLength; i++)
	{
		aun32ValidationKey[0] ^= pun32ClientIP[i];
	}


	// Seed the key with 'random' data.
	for (x = 0; x < (sizeof (aun32ValidationKey) / sizeof (uint32_t)) - 1; x++)
	{
		aun32ValidationKey[x + 1] = stiTransform (aun32ValidationKey[x]);
	} // end for

	// Now morph in the server IP
	for (int i = 0; i < nIpLength; i++)
	{
		aun32ValidationKey[0] ^= pun32ServerIP[i];
	}

	// Walk through the output buffer and randomize the result again.
	for (x = 0; x < (sizeof (aun32ValidationKey) / sizeof (uint32_t)) - 1; x++)
	{
		aun32ValidationKey[x + 1] = stiTransform (aun32ValidationKey[x]);
	} // end for

	// Now Base64 encode the result and store it in the buffer
	stiBase64Encode (
		pszKey, 
		(char *)aun32ValidationKey,
		sizeof (aun32ValidationKey));

} // end stiKeyCreate

#else

////////////////////////////////////////////////////////////////////////////////
//; stiClientValidate
//
// Description: Validates a client connection key
//
// Abstract:
//
// Returns:
//	estiOK if validation is successful, otherwise estiERROR
//
EstiResult stiClientValidate (
	IN uint32_t un32ClientIP,
	IN uint32_t un32ServerIP,
	IN const char * szValidationKey)
{
	EstiResult eResult = estiOK;

	char szCheckKey[MAX_KEY];

	// Create a client key based on the IP Addresses passed in
	stiKeyCreate (un32ClientIP, un32ServerIP, szCheckKey);

	// Compare the key generated with the key passed in
	if (0 != strcmp (szValidationKey, szCheckKey))
	{
		eResult = estiERROR;
	} // end if

	return (eResult);
} // end stiClientValidate


////////////////////////////////////////////////////////////////////////////////
//; stiKeyCreate
//
// Description: Creates a client connection key based on two IP addresses
//
// Abstract:
//	Creates a Base64 encoded 128 bit pseudo random number based upon the
//	client and server IP addresses.
//
// Returns:
//	None
//
void stiKeyCreate (
	uint32_t un32ClientIP,
	uint32_t un32ServerIP,
	char *pszKey)
{
	uint32_t aun32ValidationKey[4];
	unsigned int x;

	// Initialize the first location of the key with a transform of the
	// Client IP
	aun32ValidationKey[0] = 0x532fa3db ^ un32ClientIP;

	// Seed the key with 'random' data.
	for (x = 0; x < (sizeof (aun32ValidationKey) / sizeof (uint32_t)) - 1; x++)
	{
		aun32ValidationKey[x + 1] = stiTransform (aun32ValidationKey[x]);
	} // end for

	// Now morph in the server IP
	aun32ValidationKey[0] ^= un32ServerIP;

	// Walk through the output buffer and randomize the result again.
	for (x = 0; x < (sizeof (aun32ValidationKey) / sizeof (uint32_t)) - 1; x++)
	{
		aun32ValidationKey[x + 1] = stiTransform (aun32ValidationKey[x]);
	} // end for

	// Now Base64 encode the result and store it in the buffer
	stiBase64Encode (
		pszKey,
		(char *)aun32ValidationKey,
		sizeof (aun32ValidationKey));

} // end stiKeyCreate

#endif


////////////////////////////////////////////////////////////////////////////////
//; stiKeyCreate
//
// Description: Creates a client connection key based on a seed value.
//
// Abstract:
//	Creates a Base64 encoded 128 bit pseudo random number based upon a 
//	32 bit seed value.
//
// Returns:
//	None.
//
void stiKeyCreate (
	uint32_t un32Seed,
	char *pszKey)
{
	uint32_t aun32ValidationKey[4];
	unsigned int x;

	// Initialize the first location of the key with a transform of the 
	// seed
	aun32ValidationKey[0] = 0x37642DE9 ^ un32Seed;

	// Fill the key with 'random' data.
	for (x = 0; x < (sizeof (aun32ValidationKey) / sizeof (uint32_t)) - 1; x++)
	{
		aun32ValidationKey[x + 1] = stiTransform (aun32ValidationKey[x]);
	} // end for

	// Now randomize the first location from the last
	aun32ValidationKey[0] = stiTransform (
		aun32ValidationKey[(sizeof (aun32ValidationKey) / 
							sizeof (uint32_t)) - 1]);

	// Now Base64 encode the result and store it in the buffer
	stiBase64Encode (
		pszKey, 
		(char *)aun32ValidationKey,
		sizeof (aun32ValidationKey));

} // end stiKeyCreate


////////////////////////////////////////////////////////////////////////////////
//; stiPasswordCreate
//
// Description: Creates a client connection key based on a seed value.
//
// Abstract:
//	Creates a Base64 encoded 128 bit pseudo random number based upon a 
//	32 bit seed value. 
//
// Returns:
//	None.
//
void stiPasswordCreate (
	const char * szKey1,
	const char * szKey2,
	char *pszPassword)
{
	uint32_t aun32KeyBuffer[4];
	uint32_t aun32TempBuffer[5];
	unsigned int x;

	// Initialize the key buffers with the key data that was passed in.

	// NOTE: stiBase64Decode will decode 3 extra bytes, so make sure that
	// the buffer will hold it without stomping memory. The temp buffer
	// has the space for the extra bytes, so decode into the temp buffer, then 
	// move the "real" data into the first key buffer.
	stiBase64Decode ((char *)aun32TempBuffer, nullptr, szKey1);
	memcpy (aun32KeyBuffer, aun32TempBuffer, sizeof (aun32KeyBuffer));

	// Now decode the second key
	stiBase64Decode ((char *)aun32TempBuffer, nullptr, szKey2);

	// Walk through the key buffer and create a transform of the two keys
	for (x = 0; x < (sizeof (aun32KeyBuffer) / sizeof (uint32_t)); x++)
	{
		aun32KeyBuffer[x] ^= stiTransform (aun32TempBuffer[x]);
	} // end for

	// Walk through the output buffer and randomize the result again.
	for (x = 0; x < (sizeof (aun32KeyBuffer) / sizeof (uint32_t)) - 1; x++)
	{
		aun32KeyBuffer[x + 1] ^= stiTransform (aun32KeyBuffer[x]);
	} // end for

	// Now randomize the first location from the last
	aun32KeyBuffer[0] = stiTransform (
		aun32KeyBuffer[(sizeof (aun32KeyBuffer) / 
							sizeof (uint32_t)) - 1]);

	// Now Base64 encode the result and store it in the buffer
	stiBase64Encode (
		pszPassword, 
		(char *)aun32KeyBuffer,
		sizeof (aun32KeyBuffer));
	
} // end stiPasswordCreate


////////////////////////////////////////////////////////////////////////////////
//; stiPasswordValidate
//
// Description: Validates a password from a client connection key
//
// Abstract:
//
// Returns:
//	estiOK if validation is successful, otherwise estiERROR
//
EstiResult stiPasswordValidate (
	IN const char * szValidationKey1,
	IN const char * szValidationKey2,
	IN const char * szPassword)
{
	EstiResult eResult = estiOK;

	char szCheckPassword[MAX_PASSWORD];
	
	// Create a client key based on the IP Addresses passed in
	stiPasswordCreate (szValidationKey1, szValidationKey2, szCheckPassword);

	// Compare the key generated with the key passed in
	if (0 != strcmp (szPassword, szCheckPassword))
	{
		eResult = estiERROR;
	} // end if

	return (eResult);
} // end stiPasswordValidate


////////////////////////////////////////////////////////////////////////////////
//; stiTransform
//
// Description: Generates a pseudo-random transform
//
// Abstract:
//	Returns a pseudo-random transform based on the seed number. This routine 
//	is roughly based on the MD5 random number generator
//
// Returns:
//	uint32_t - pseudo random transform
//
uint32_t stiTransform (
	uint32_t un32Seed)
{
	uint32_t a, b, c, d;

	a = 0x67452301 + un32Seed;
	b = 0xefcdab89;
	c = 0x98badcfe;
	d = 0x10325476;

	/* The four core functions - F1 is optimized somewhat */
	/* #define F1(x, y, z) (x & y | ~x & z) */
	#define F1(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
	#define F2(x, y, z) F1((z), (x), (y))
	#define F3(x, y, z) ((x) ^ (y) ^ (z))
	#define F4(x, y, z) ((y) ^ ((x) | ~(z)))

	#define MD5STEP(f,w,x,y,z,in,s) \
			 ((w) += f((x),(y),(z)) + (in), (w) = ((w)<<s | (w)>>(32-(s))) + (x)) /* NOLINT (bugprone-macro-parentheses) */

	MD5STEP(F1, a, b, c, d, 0xd76aa478, 7);
	MD5STEP(F1, d, a, b, c, 0xe8c7b756, 12);
	MD5STEP(F1, c, d, a, b, 0x242070db, 17);
	MD5STEP(F1, b, c, d, a, 0xc1bdceee, 22);
	MD5STEP(F1, a, b, c, d, 0xf57c0faf, 7);
	MD5STEP(F1, d, a, b, c, 0x4787c62a, 12);
	MD5STEP(F1, c, d, a, b, 0xa8304613, 17);
	MD5STEP(F1, b, c, d, a, 0xfd469501, 22);
	MD5STEP(F1, a, b, c, d, 0x698098d8, 7);
	MD5STEP(F1, d, a, b, c, 0x8b44f7af, 12);
	MD5STEP(F1, c, d, a, b, 0xffff5bb1, 17);
	MD5STEP(F1, b, c, d, a, 0x895cd7be, 22);
	MD5STEP(F1, a, b, c, d, 0x6b901122, 7);
	MD5STEP(F1, d, a, b, c, 0xfd987193, 12);
	MD5STEP(F1, c, d, a, b, 0xa679438e, 17);
	MD5STEP(F1, b, c, d, a, 0x49b40821, 22);

	MD5STEP(F2, a, b, c, d, 0xf61e2562, 5);
	MD5STEP(F2, d, a, b, c, 0xc040b340, 9);
	MD5STEP(F2, c, d, a, b, 0x265e5a51, 14);
	MD5STEP(F2, b, c, d, a, 0xe9b6c7aa, 20);
	MD5STEP(F2, a, b, c, d, 0xd62f105d, 5);
	MD5STEP(F2, d, a, b, c, 0x02441453, 9);
	MD5STEP(F2, c, d, a, b, 0xd8a1e681, 14);
	MD5STEP(F2, b, c, d, a, 0xe7d3fbc8, 20);
	MD5STEP(F2, a, b, c, d, 0x21e1cde6, 5);
	MD5STEP(F2, d, a, b, c, 0xc33707d6, 9);
	MD5STEP(F2, c, d, a, b, 0xf4d50d87, 14);
	MD5STEP(F2, b, c, d, a, 0x455a14ed, 20);
	MD5STEP(F2, a, b, c, d, 0xa9e3e905, 5);
	MD5STEP(F2, d, a, b, c, 0xfcefa3f8, 9);
	MD5STEP(F2, c, d, a, b, 0x676f02d9, 14);
	MD5STEP(F2, b, c, d, a, 0x8d2a4c8a, 20);

	MD5STEP(F3, a, b, c, d, 0xfffa3942, 4);
	MD5STEP(F3, d, a, b, c, 0x8771f681, 11);
	MD5STEP(F3, c, d, a, b, 0x6d9d6122, 16);
	MD5STEP(F3, b, c, d, a, 0xfde5380c, 23);
	MD5STEP(F3, a, b, c, d, 0xa4beea44, 4);
	MD5STEP(F3, d, a, b, c, 0x4bdecfa9, 11);
	MD5STEP(F3, c, d, a, b, 0xf6bb4b60, 16);
	MD5STEP(F3, b, c, d, a, 0xbebfbc70, 23);
	MD5STEP(F3, a, b, c, d, 0x289b7ec6, 4);
	MD5STEP(F3, d, a, b, c, 0xeaa127fa, 11);
	MD5STEP(F3, c, d, a, b, 0xd4ef3085, 16);
	MD5STEP(F3, b, c, d, a, 0x04881d05, 23);
	MD5STEP(F3, a, b, c, d, 0xd9d4d039, 4);
	MD5STEP(F3, d, a, b, c, 0xe6db99e5, 11);
	MD5STEP(F3, c, d, a, b, 0x1fa27cf8, 16);
	MD5STEP(F3, b, c, d, a, 0xc4ac5665, 23);

	MD5STEP(F4, a, b, c, d, 0xf4292244, 6);
	MD5STEP(F4, d, a, b, c, 0x432aff97, 10);
	MD5STEP(F4, c, d, a, b, 0xab9423a7, 15);
	MD5STEP(F4, b, c, d, a, 0xfc93a039, 21);
	MD5STEP(F4, a, b, c, d, 0x655b59c3, 6);
	MD5STEP(F4, d, a, b, c, 0x8f0ccc92, 10);
	MD5STEP(F4, c, d, a, b, 0xffeff47d, 15);
	MD5STEP(F4, b, c, d, a, 0x85845dd1, 21);
	MD5STEP(F4, a, b, c, d, 0x6fa87e4f, 6);
	MD5STEP(F4, d, a, b, c, 0xfe2ce6e0, 10);
	MD5STEP(F4, c, d, a, b, 0xa3014314, 15);
	MD5STEP(F4, b, c, d, a, 0x4e0811a1, 21);
	MD5STEP(F4, a, b, c, d, 0xf7537e82, 6);
	MD5STEP(F4, d, a, b, c, 0xbd3af235, 10);
	MD5STEP(F4, c, d, a, b, 0x2ad7d2bb, 15);
	MD5STEP(F4, b, c, d, a, 0xeb86d391, 21);

	return (a);
} // end stiTransform


// end file stiClientValidate.cpp
