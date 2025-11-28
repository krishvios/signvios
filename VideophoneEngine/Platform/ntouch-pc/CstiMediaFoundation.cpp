//*****************************************************************************
//
// FileName:        CstiMediaFoundation.cpp
//
// Abstract:        A helper class for dealing with Microsoft Media Foundation
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "CstiMediaFoundation.h"



CstiMediaFoundation::CstiMediaFoundation ()
{
	HRESULT hr = S_OK;
	hr = MFStartup (MF_VERSION);
	if (hr == S_OK)
	{
		H264Decoders = GetDecoder (MFVideoFormat_H264);
		H265Decoders = GetDecoder (MFVideoFormat_HEVC);
		H264Encoders = GetEncoder (MFVideoFormat_H264);
		H265Encoders = GetEncoder (MFVideoFormat_HEVC);
	}
}


CstiMediaFoundation::~CstiMediaFoundation ()
{
}

std::vector<IMFActivate*> CstiMediaFoundation::GetEncoder (GUID codec)
{
	HRESULT hr = S_OK;
	IMFActivate **ppActivate = NULL;    // Array of activation objects.
	MFT_REGISTER_TYPE_INFO outputType = { MFMediaType_Video, codec };
	UINT32 count = 0;
	UINT32 unFlags = MFT_ENUM_FLAG_SORTANDFILTER;
	// Array of media types.
	//MFT_REGISTER_TYPE_INFO inputType = { MFMediaType_Video, MFVideoFormat_I420 };
	
	hr = MFTEnumEx (
		MFT_CATEGORY_VIDEO_ENCODER,
		unFlags,
		NULL,		// Input type
		&outputType,    // Output type
		&ppActivate,
		&count
		);

	std::vector<IMFActivate*> activeResults;
	if (hr == S_OK)
	{
		for (int i = 0; i < count; i++)
		{
			activeResults.push_back (ppActivate[i]);
		}
	}

	CoTaskMemFree (ppActivate);

	return activeResults;
}

std::vector<IMFActivate*> CstiMediaFoundation::GetDecoder (GUID codec)
{
	HRESULT hr = S_OK;
	IMFActivate **ppActivate = NULL;    // Array of activation objects.
	MFT_REGISTER_TYPE_INFO inputType = { MFMediaType_Video, codec };
	UINT32 count = 0;
	UINT32 unFlags = MFT_ENUM_FLAG_SORTANDFILTER;
	// Array of media types.S
	//MFT_REGISTER_TYPE_INFO outputType = { MFMediaType_Video, MFVideoFormat_I420 };

	hr = MFTEnumEx (
		MFT_CATEGORY_VIDEO_DECODER,
		unFlags,
		&inputType,	   // Input type
		NULL,    // Output type
		&ppActivate,
		&count
		);
	std::vector<IMFActivate*> activeResults;
	if (hr == S_OK)
	{
		for (int i = 0; i < count; i++)
		{
			activeResults.push_back (ppActivate[i]);
		}
	}

	CoTaskMemFree (ppActivate);

	return activeResults;
}
