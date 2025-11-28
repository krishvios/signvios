////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Module Name: utf8EncodeDecode
//
//  File Name: utf8EncodeDecode.h
//
//	Abstract:
//		Helper functions for conversion between UTF-8 and Unicode arrays
//
////////////////////////////////////////////////////////////////////////////////
#ifndef UTF8ENCODEDECODE_H
#define UTF8ENCODEDECODE_H


#include "stiSVX.h"

#define un16UNICODE_CR						(uint16_t)0x000D // Carriage Return (expect to be followed by un16UNICODE_LF).  Preferred is un16UNICODE_NL
#define un16UNICODE_BEL	 					(uint16_t)0x0007 // Indication of a bell or some other alert
#define un16UNICODE_BYTE_ORDER_MARK	(uint16_t)0xfeff // Sent at the beginning of the channel initialization
#define un16UNICODE_ESC						(uint16_t)0x001b // Escape
#define un16UNICODE_INT						(uint16_t)0x0061 // Only is an Interrupt if preceeded by un16UNICODE_ESC
#define un16UNICODE_LF						(uint16_t)0x000a // Line Feed (expect to be immediately preceeded by un16UNICODE_CR
#define un16UNICODE_MISSING_CHAR		(uint16_t)0xfffd // Sent to the UI to inform of missing text.
#define un16UNICODE_NL						(uint16_t)0x2028 // New Line (preferred over <CR><LF>
#define un16UNICODE_SGR						(uint16_t)0x009b // Screen Graphics Rendition
#define un16UNICODE_SGR_TERMINATOR		(uint16_t)0x006d // SGR terminator.
#define un16UNICODE_SOS						(uint16_t)0x0098 // Start of String ... used to send control functions.
#define un16UNICODE_ST						(uint16_t)0x009c // String terminator.  Follows the end of a sequence that begins with un16UNICODE_S0S


int UTF8ToUnicodeConvert (uint16_t *pun16Dest, const uint8_t *pun8Src, int nLength);
int UnicodeToUTF8Convert (uint8_t *pun8Dest, const uint16_t *pun16Src);
int UTF8LengthGet (const uint8_t *pun8Buffer, int nLength);

#endif
