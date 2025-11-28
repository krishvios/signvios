// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#include "utf8EncodeDecode.h"

#define UTF8_ONE_BYTE_MASK 0x0
#define UTF8_TWO_BYTE_MASK 0X6  // 1100 0000 right-shifted by 5
#define UTF8_THREE_BYTE_MASK 0xE // 1110 0000 right-shifted by 4
#define UTF8_FOUR_BYTE_MASK 0x1E // 1111 0000 right-shifted by 3


/*!
 * \brief UTF8ToUnicodeConvert
 * This method decodes a UTF8 array into a Unicode array.
 *
 * \param pun16Dest - the buffer in which to place the Unicode-encoded array.
 * \param pun8Src - the buffer containing the UTF8 array to be converted.
 * \param nLength - the number of bytes in the UTF8 array.
 *
 * \return The number of unicode characters decoded or -1 on failure.
 */
int UTF8ToUnicodeConvert (uint16_t *pun16Dest, const uint8_t *pun8Src, int nLength)
{
	uint8_t un8Copy;
	int nRet = 0;

	for (; nRet != -1 && nLength > 0; nRet++)
	{
		un8Copy = *pun8Src;

		if (UTF8_ONE_BYTE_MASK == (un8Copy >> 7))
		{
			// One-byte encoding
			*pun16Dest = un8Copy;
		}

		else if (UTF8_TWO_BYTE_MASK == (un8Copy >> 5))
		{
			// Two-byte encoding
			*pun16Dest = (un8Copy & 0x1f);

			// Move this left 6 bits to make room for the 6 bits from the second byte.
			*pun16Dest <<= 6;

			// Now move the 6 bits in from the next byte.
			pun8Src++;
			nLength--;
			un8Copy = *pun8Src;
			*pun16Dest |= (un8Copy & 0x3f);
		}

		else if (UTF8_THREE_BYTE_MASK == (un8Copy >> 4))
		{
			// Three-byte encoding
			*pun16Dest = (un8Copy & 0x0f);

			// Move this left 6 bits to make room for the 6 bits from the second byte.
			*pun16Dest <<= 6;

			// Now move the 6 bits in from the next byte.
			pun8Src++;
			un8Copy = *pun8Src;
			*pun16Dest |= (un8Copy & 0x3f);

			// Move this left 6 bits to make room for the 6 bits from the third byte.
			*pun16Dest <<= 6;

			// Now move the 6 bits in from the next byte.
			pun8Src++;
			un8Copy = *pun8Src;
			*pun16Dest |= (un8Copy & 0x3f);

			nLength -= 2;
		}

		else if (UTF8_FOUR_BYTE_MASK == (un8Copy >> 3))
		{
			stiTrace ("<UTF8ToUnicodeConvert> Can't decode a UTF8-encoded character that is larger than 3-bytes into a Unicode character.  Skipping 4 bytes.\n");
			pun8Src += 3;  // The other byte is skipped as part of the loop.
			nLength -= 3;

		}
		else
		{
			stiTrace ("<UTF8ToUnicodeConvert> Failure decoding.  Aborting ...\n");
			nRet = -1;
		}

		pun8Src ++;
		pun16Dest ++;
		nLength--;
	}

	return nRet;
}


/*!
 * \brief UnicodeToUTF8Convert
 * This method encodes a Unicode string to UTF8.
 *
 * \param pun8Dest - the buffer in which to place the UTF8-encoded array.
 * \param pun16Src - the buffer containing the Unicode character array to be encoded.
 *
 * \return The number of bytes it took to UTF-8 encode it.
 */
int UnicodeToUTF8Convert (uint8_t *pun8Dest, const uint16_t *pun16Src)
{
	int nRet = 0;
	
	// Did we get valid pointers passed in?
	if (pun8Dest && pun16Src)
	{
		uint8_t un8Buffer[3];
		uint16_t un16Char;

		// Loop through the source array
		for (; *pun16Src;)
		{
			un16Char = *pun16Src;

			// Determine the number of octets required for the current Unicode character
			if (*pun16Src < 0x0080)
			{
				// Just copy it, no encoding is necessary for this character.
				*pun8Dest = (uint8_t)*pun16Src;
				nRet++;
			} // end if

			else if (*pun16Src < 0x0800)
			{
				// This character will only take two bytes to encode

				// Prepare the high-order bits of the octets
				un8Buffer[0] = 0xc0;
				un8Buffer[1] = 0x80;

				// Fill in the rest of the bits in the needed buffers.
				un8Buffer[1] |= (un16Char & 0x3f);

				// Get rid of the lowest order 6 bits
				un16Char >>= 6;

				un8Buffer[0] |= (un16Char & 0x1f);

				// Now store it in the destination array
				*pun8Dest = un8Buffer[0];
				pun8Dest++;
				*pun8Dest = un8Buffer[1];

				nRet += 2;
			} // end else if

			else
			{
				// Make sure it isn't coming from the reserved (UTF-16) area (0xd800 - 0xdfff)
				if (*pun16Src > 0xd7ff && *pun16Src < 0xe000)
				{
					stiTrace ("<UnicodeToUTF8Convert> Source character is from prohibited area (U+D800 - U+DFFF), not encoding\n");
				} // end if
				else
				{
					// This character will take three bytes to encode

					// Prepare the high-order bits of the octets
					un8Buffer[0] = 0xe0;
					un8Buffer[1] = 0x80;
					un8Buffer[2] = 0x80;

					// Fill in the rest of the bits in the needed buffers.
					un8Buffer[2] |= (un16Char & 0x3f);

					// Get rid of the lowest order 6 bits
					un16Char >>= 6;

					un8Buffer[1] |= (un16Char & 0x3f);

					// Get rid of the lowest order 6 bits
					un16Char >>= 6;

					un8Buffer[0] |= (un16Char & 0xf);

					// Now store it in the destination array
					*pun8Dest = un8Buffer[0];
					pun8Dest++;
					*pun8Dest = un8Buffer[1];
					pun8Dest++;
					*pun8Dest = un8Buffer[2];

					nRet += 3;
				} // end else
			} // end else

			// Move to the next
			pun16Src++;
			pun8Dest++;
		} // end for
	} // end if
	else
	{
		// Invalid pointers passed in.
		stiTrace ("<UnicodeToUTF8Convert> pointers passed in are invalid\n");
	}

	return nRet;
}


/*!
 * \brief UTF8LengthGet
 * This method does a quick parse of the UTF8 coded array and determines the number of unicode characters
 * that are encoded therein.
 *
 * \param pun8Buffer - the buffer containing the UTF8-encoded array.
 * \param nLength - the length of the encoded array in bytes
 *
 * \return The number of unicode characters encoded within the UTF-8 array.
 */
int UTF8LengthGet (const uint8_t *pun8Buffer, int nLength)
{
	int nRet = 0;
	const uint8_t *pun8Current;

	if (pun8Buffer && nLength > 0)
	{
		int nIndex = 0;
		uint8_t un8Copy;

		do
		{
			pun8Current = pun8Buffer + nIndex;
			un8Copy = *pun8Current;

			if (UTF8_ONE_BYTE_MASK == (un8Copy >> 7))
			{
				// One-byte encoding
				nRet ++;
				nIndex++;
			}

			else if (UTF8_TWO_BYTE_MASK == (un8Copy >> 5))
			{
				// Two-byte encoding
				nRet ++;
				nIndex += 2;
			}

			else if (UTF8_THREE_BYTE_MASK == (un8Copy >> 4))
			{
				// Three-byte encoding
				nRet ++;
				nIndex += 3;
			}

			else if (UTF8_FOUR_BYTE_MASK == (un8Copy >> 3))
			{
				// Four-byte encoding
				nRet ++;
				nIndex += 4;
			}
			else
			{
				//
				// We should never get here (unless the data we are working with is invalid).
				//
				stiASSERT (estiFALSE);
				nRet = 0;
				break;
			}
		} while (nIndex < nLength && nRet > 0);

		//
		// If the encoding was correct nIndex should equal nLength.  If it doesn't
		// then something is wrong with the data.  Assert and set the return value to zero.
		//
		if (nIndex != nLength)
		{
			stiASSERT (estiFALSE);
			nRet = 0;
		}
	}

	return nRet;
}
