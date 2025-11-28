//*****************************************************************************
//
// FileName:        CstiMediaFoundation.h
//
// Abstract:        A helper class for dealing with Microsoft Media Foundation
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#pragma once

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "ole32")
#pragma comment(lib, "strmiids")

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <dshow.h>
#include <wmcodecdsp.h>
#include <codecapi.h>
#include <vector>	   

#define stiMFTest(errorText, errorResult) if (FAILED(hr)) { stiTrace(errorText); hResult = stiMAKE_ERROR(errorResult); goto STI_BAIL; }

class CstiMediaFoundation
{
public:
	CstiMediaFoundation ();
	~CstiMediaFoundation ();

	std::vector<IMFActivate*> H265Encoders;
	std::vector<IMFActivate*> H264Encoders;

	std::vector<IMFActivate*> H265Decoders;
	std::vector<IMFActivate*> H264Decoders;

private:
	std::vector<IMFActivate*> GetEncoder (GUID codec);
	std::vector<IMFActivate*> GetDecoder (GUID codec);
};

namespace ntouchPC
{
	CstiMediaFoundation MediaFoundation;
}
