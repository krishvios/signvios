////////////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  stiPayloadTypes.cpp
//  Created on:      03-Mar-2011
////////////////////////////////////////////////////////////////////////////////////////
#include "stiPayloadTypes.h"


////////////////////////////////////////////////////////////////////////////////
//; NextDynamicPayloadTypeGet
//
// Description: Return the next payload type
//
// Abstract:
//	This method returns the next dynamic payload type to use, wrapping around
//	to the beginning type once we have reached the end type.
//
// Returns:
//	The next dynamic payload type to use
//
int8_t NextDynamicPayloadTypeGet ()
{
	static int8_t n8NextPayload = n8DYNAMIC_PAYLOAD_BEGIN;

	int8_t n8Return = n8NextPayload;

	// Check to see if we should wrap back to the beginning or not
	if (n8NextPayload >= n8DYNAMIC_PAYLOAD_END)
	{
		n8NextPayload = n8DYNAMIC_PAYLOAD_BEGIN;
	}
	else
	{
		n8NextPayload++;

		//
		// Some vendors are erroneously expecting dtmf to be sent as payload type 101. Skip
		// over this if reached.
		//
		if (n8NextPayload == PAYLOAD_TYPE_DTMF)
		{
			n8NextPayload++;
		}
	}

	return n8Return;
}
