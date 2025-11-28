////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	stiBase64
//
//	File Name:	stiBase64.cpp
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		Encodes and decodes binary data to/from Base64. Algorithm originally 
//		found at:
//			http://www.experts-exchange.com/Programming/
//			Programming_Languages/Cplusplus/Q_20079662.html
//		No attribution or licensing requirements were given.
//
////////////////////////////////////////////////////////////////////////////////

//
// Includes
//

#include "stiBase64.h"

#include <cstring>
#include "stiDefs.h"

#include <sstream>
#include <iomanip>

//
// Locals
//
static const char * g_pchBase64Encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

//
// Function Declarations
//

////////////////////////////////////////////////////////////////////////////////
//; stiBase64Encode
//
// Description: Encodes a memory buffer into the provided string buffer
//
// Abstract:
//
// Returns:
//	None
//
void stiBase64Encode (
	OUT char * pchOutput, 
	IN const char * pchInput, 
	IN int nInputSize)
{
	int nNumEncoded = 0;
	int nIdx[4];  // will contain the indices of coded letters in 
	// g_pchBase64Encoding string; valid values [0..64]; the value
	// of 64 has special meaning - the padding symbol

	// process the data (3 bytes of input provide 4 bytes of output)
	while (nNumEncoded < nInputSize)
	{
		nIdx[0] = ((*pchInput) & 0xFC)>>2;
		nIdx[1] = ((*pchInput) & 0x03)<<4;
		pchInput++;
		nNumEncoded++;
		if (nNumEncoded < nInputSize)
		{
			nIdx[1] |= ((*pchInput) & 0xF0)>>4;
			nIdx[2]  = ((*pchInput) & 0x0F)<<2;
			pchInput++;
			nNumEncoded++;
			if (nNumEncoded < nInputSize)
			{
				nIdx[2] |= ((*pchInput) & 0xC0) >> 6;
				nIdx[3]  = (*pchInput) & 0x3F;
				pchInput++;
				nNumEncoded++;
			} // end if
			else
			{
				nIdx[3] = 64;
			} // end else
		} // end if
		else
		{
			// refer to padding symbol '='
			nIdx[2] = 64;
			nIdx[3] = 64;
		} // end else

		*(pchOutput+0) = *(g_pchBase64Encoding + nIdx[0]);
		*(pchOutput+1) = *(g_pchBase64Encoding + nIdx[1]);
		*(pchOutput+2) = *(g_pchBase64Encoding + nIdx[2]);
		*(pchOutput+3) = *(g_pchBase64Encoding + nIdx[3]);
		pchOutput += 4;
	} // end while

	// set this to terminate output string
	*pchOutput = '\0';

	return;
} // end stiBase64Encode


////////////////////////////////////////////////////////////////////////////////
//; stiBase64Decode
//
// Description: Decodes the given Base64 encoded string into a memory buffer
//
// Abstract:
//
// Returns:
//	None
//
void stiBase64Decode (
	OUT char * pchOutput,
	OUT int *pnLength,
	IN const char * pchInput)
{
	int nLength = 0;
	int nIdx[4];  // will contain the indices of coded letters in 
	// g_pchBase64Encoding string; valid values [0..64]; the value
	// of 64 has special meaning - the padding symbol

	// process the data (4 bytes of input provide 3 bytes of output
	while ('\0' != *pchInput)
	{
		nIdx[0] = nIdx[1] = nIdx[2] = nIdx[3] = 64;
		nIdx[0] = (
			strchr (g_pchBase64Encoding, (*pchInput)) - 
				g_pchBase64Encoding);
		pchInput++;
		if ('\0' != *pchInput)
		{
			nIdx[1] = (
				strchr (g_pchBase64Encoding, (*pchInput)) - 
					g_pchBase64Encoding);
			pchInput++;
			if ('\0' != (*pchInput))
			{
				nIdx[2] = (
					strchr (g_pchBase64Encoding, (*pchInput)) - 
						g_pchBase64Encoding);
				pchInput++;
				if ('\0' != (*pchInput))
				{
					nIdx[3] = (
						strchr (g_pchBase64Encoding, (*pchInput)) - 
							g_pchBase64Encoding);
					pchInput++;
				} // end if
			} // end if
		} // end if

		if (nIdx[2] == 64 && nIdx[3] == 64)
		{
			*(pchOutput+0) = (char)((nIdx[0]<<2) | (nIdx[1]>>4));

			nLength += 1;
		}
		else if (nIdx[3] == 64)
		{
			*(pchOutput+0) = (char)((nIdx[0]<<2) | (nIdx[1]>>4));
			*(pchOutput+1) = (char)((nIdx[1]<<4) | (nIdx[2]>>2));

			nLength += 2;
		}
		else
		{
			*(pchOutput+0) = (char)((nIdx[0]<<2) | (nIdx[1]>>4));
			*(pchOutput+1) = (char)((nIdx[1]<<4) | (nIdx[2]>>2));
			*(pchOutput+2) = (char)((nIdx[2]<<6) | nIdx[3]);

			nLength += 3;
		}

		pchOutput += 3;
	} // end while

	// set this to terminate output string
	*pchOutput = '\0';

	if (pnLength)
	{
		*pnLength = nLength;
	}

	return;
} // end stiBase64Decode

namespace vpe
{

/*!
 * \brief base64 encode std::string interface
 * \param string to encode
 * \return encoded string
 */
std::string base64Encode(const std::string &input)
{
	size_t encodedSize = input.size();

	// Make sure the input size is padded to 3 bytes
	if(input.size() % 3 != 0)
	{
		encodedSize += 3 - (input.size() % 3);
	}

	// base64 uses 4 bytes to encode 3 bytes
	encodedSize = encodedSize / 3 * 4;

	// Add an additional byte for the null char that it puts on
	++encodedSize;

	std::string ret;
	ret.resize(encodedSize);

	stiBase64Encode(&ret[0], input.c_str(), input.size());

	// Remove redundant null char
	ret.pop_back();

	return ret;
}

/*!
 * \brief base64 decode std::string interface
 * \param string to decode
 * \return decoded string
 */
std::string base64Decode(const std::string &input)
{
	std::string ret;
	int finalSize(0);

	// The resultant string will be smaller than this, but let's just start here
	ret.resize(input.size());

	stiBase64Decode(&ret[0], &finalSize, input.data());

	ret.resize(finalSize);

	return ret;
}

std::string hexEncode (const std::string &input)
{
	std::stringstream ss;

	ss << std::hex << std::setw(2) << std::setfill('0');

	for(size_t i = 0; i < input.size(); ++i)
	{
		ss << (int)(uint8_t)input[i];
	}

	return ss.str();
}

std::string hexDecode (const std::string &input)
{
	// TODO: implement me
	return input;
}

}


// end file stiBase64.cpp
