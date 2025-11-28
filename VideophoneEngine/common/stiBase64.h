#pragma once

////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//	Class Name:	stiBase64
//
//	File Name:	stiBase64.h
//
//	Owner:		Scot L. Brooksby
//
//	Abstract:
//		See stiBase64.cpp
//
////////////////////////////////////////////////////////////////////////////////

#include <string>

extern void stiBase64Encode (char * pchOutput, const char * pchInput, int nInputSize);
extern void stiBase64Decode (char * pchOutput, int *pnLength, const char * pchInput);

namespace vpe
{

// Some nice std::string wrappers
std::string base64Encode (const std::string &input);
std::string base64Decode (const std::string &input);

// hex encode/decode (probably belongs in a different file, or maybe this should be renamed)
std::string hexEncode (const std::string &input);
std::string hexDecode (const std::string &input);

} // vpe namespace

