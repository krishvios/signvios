//*****************************************************************************
//
// FileName:        CstiMF265Encoder.cpp
//
// Abstract:        Declaration of the CstiMF265Encoder class used to encode
//					H.265 videos with the MediaFoundation 265 encoder.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "CstiMF265Encoder.h"

CstiMF265Encoder::CstiMF265Encoder (IMFTransform *pEncoder, ICodecAPI* pCodecAPI) :
	m_pEncoder (pEncoder),
	m_pCodecAPI (pCodecAPI),
	m_eProfile (EstiVideoProfile::estiH265_PROFILE_MAIN),
	m_unLevel (120),
	m_nTimeStamp (0)
{
	SetPropertyInt (CODECAPI_AVLowLatencyMode, 1);
	SetPropertyInt (CODECAPI_AVEncCommonQualityVsSpeed, 0);
}


CstiMF265Encoder::~CstiMF265Encoder ()
{
	if (m_pCodecAPI)
	{
		m_pCodecAPI->Release ();
	}

	if (m_pEncoder)
	{
		m_pEncoder->Release ();
	}
}


//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::CreateVideoEncoder
//
//  Description: Static function used to create a CstiMF265Encoder
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CstiMF265Encoder is created.
//		stiRESULT_MEMORY_ALLOC_ERROR if CstiMF265Encoder creation fails.
//
stiHResult CstiMF265Encoder::CreateVideoEncoder (CstiMF265Encoder **pMF265Encoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HRESULT hr = S_OK;
	IMFTransform *pEncoder = NULL;
	ICodecAPI* pCodecAPI = NULL;
	for (std::vector<IMFActivate *>::const_iterator i = ntouchPC::MediaFoundation.H265Encoders.begin (); i != ntouchPC::MediaFoundation.H265Encoders.end (); i++)
	{
		IMFActivate * iter = (*i);
		hr = iter->ActivateObject (IID_PPV_ARGS (&pEncoder));
		if (SUCCEEDED (hr))
		{
			hr = pEncoder->QueryInterface<ICodecAPI> (&pCodecAPI);
			if (SUCCEEDED (hr))
			{
				break;
			}
			pEncoder->Release ();
			pEncoder = NULL;
		}
		else
		{
			pEncoder == NULL;
		}
	}

	if (pEncoder)
	{
		*pMF265Encoder = new CstiMF265Encoder (pEncoder, pCodecAPI);
	}
	else
	{
		hResult = stiMAKE_ERROR (stiRESULT_ERROR);//We couldn't find a valid encoder
	}


STI_BAIL:

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::CreateX265Encoder
//
//  Description: Compresses a video frame of the appropriate colorspace
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CstiX265Encoder is created.
//		stiRESULT_MEMORY_ALLOC_ERROR if CstiX265Encoder creation failes.
//
stiHResult CstiMF265Encoder::AvVideoCompressFrame (uint8_t *pInputFrame, FrameData *pOutputData, EstiColorSpace colorSpace)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HRESULT hr = S_OK;
	IMFMediaBuffer * inputBuffer = NULL;
	IMFSample *inputSample = NULL;
	MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
	DWORD status;
	IMFAttributes* outputAttributes = NULL;
	UINT32 nDiscontinuity = 0;
	stiTESTCOND (colorSpace == EstiColorSpace::estiCOLOR_SPACE_I420, stiERROR);
	UINT32 un32Len = m_n32VideoWidth * m_n32VideoHeight * 1.5;
	//----------------Input Sample------------------------
	hr = MFCreateSample (&inputSample);
	stiMFTest ("H265 Encoder: MFCreateSample", stiRESULT_ERROR);
	CstiMFBufferWrapper::Create (&inputBuffer, pInputFrame, un32Len);
	stiMFTest ("H265 Encoder: CstiMFBufferWrapper::Create", stiRESULT_ERROR);
	hr = inputSample->AddBuffer (inputBuffer);
	stiMFTest ("H265 Encoder: AddBuffer", stiRESULT_ERROR);
	long long nDuration = (10000000 / m_fFrameRate);
	hr = inputSample->SetSampleDuration (nDuration);
	stiMFTest ("H265 Encoder: SetSampleDuration", stiRESULT_ERROR);
	hr = inputSample->SetSampleTime (m_nTimeStamp);
	stiMFTest ("H265 Encoder: SetSampleTime", stiRESULT_ERROR);
	m_nTimeStamp += nDuration;
	hr = m_pEncoder->ProcessInput (0, inputSample, NULL);
	stiMFTest ("H265 Encoder: Failed Process Input", stiRESULT_ERROR);
	//----------------Output Buffer------------------------

	//----------------------Create Output Buffer-------------------------------------------
	IMFSample *outputSample;
	MFCreateSample (&outputSample);
	IMFMediaBuffer * pMediaBuffer;
	const int bufferSize = 1920 * 1080 * 2; // 750,000 / 15 fps
	uint8_t* pPictureBytes = new uint8_t[bufferSize];
	CstiMFBufferWrapper::Create (&pMediaBuffer, pPictureBytes, bufferSize);
	outputSample->AddBuffer (pMediaBuffer);
	m_pMediaBuffer = pMediaBuffer;
	//-------------------------------------------------------------------------------------

	outputDataBuffer.dwStreamID = 0;
	outputDataBuffer.pSample = outputSample;
	outputDataBuffer.dwStatus = 0;
	outputDataBuffer.pEvents = NULL;
	hr = m_pEncoder->ProcessOutput (NULL, 1, &outputDataBuffer, &status);
	stiMFTest("H265 Encoder: Failed to Process Output", stiRESULT_ERROR);

	DWORD bufferLength;
	pMediaBuffer->GetCurrentLength (&bufferLength);
	pOutputData->AddFrameData (pPictureBytes, bufferLength);
	pOutputData->SetFrameFormat (EstiVideoFrameFormat::estiBYTE_STREAM);
	

	ULONG sampleAddRef = outputSample->Release ();

	ULONG bufferAddRef = pMediaBuffer->Release ();

	//pOutputData->SetKeyFrame ();
	for (int i = 0; SUCCEEDED (hr); i++)
	{
		GUID key;
		PROPVARIANT var;
		hr = outputSample->GetItemByIndex (i, &key, &var);
		stiTrace ("%d-%d-%d-%s", key.Data1, key.Data2, key.Data3, key.Data4);
	}
	

STI_BAIL:
	if (inputSample)
	{
		ULONG reference = inputSample->Release ();
		assert (reference == 0);
	}
	if (inputBuffer)
	{
		//We don't want our buffer to be deleted.
		((CstiMFBufferWrapper*)inputBuffer)->m_pbData = NULL;
		ULONG reference = inputBuffer->Release ();
		assert (reference == 0);
	}
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::AvEncoderClose
//
//  Description: Closes the Video file.
//
//  Abstract:
// 
//  Returns:
//		S_OK if file is closed sucessfully.
//		S_FALSE if closing file failes.
//
stiHResult CstiMF265Encoder::AvEncoderClose ()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	//if (m_pOutputSample)
	//{
	//	ULONG references = m_pOutputSample->Release ();
	//	assert (references == 0);
	//	m_pOutputSample = NULL;
	//}

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::AvInitializeEncoder
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if Media Foundation 265 is initialized.
//		stiRESULT_ERROR if Media Foundation 265 initialize fails.
//
stiHResult CstiMF265Encoder::AvInitializeEncoder ()
{
	return AvUpdateEncoderSettings ();
}

//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::AvUpdateEncoderSettings
//
//  Description: Setup the input and output type for the encoder
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if settings are updated
//		stiRESULT_ERROR if updating of settings fails.
//
stiHResult CstiMF265Encoder::AvUpdateEncoderSettings ()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HRESULT hr = S_OK;
	IMFMediaType * inputType;
	DWORD flags = 0;
	IMFMediaType * outputType;
	////--------------------Setup Output Type------------------------------------
	hr = MFCreateMediaType (&outputType);
	stiMFTest ("H265 Encoder: Failed Creating Output Media Type", stiRESULT_ERROR);
	hr = outputType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Video);
	stiMFTest ("H265 Encoder: Failed Setting Output Major Type", stiRESULT_ERROR);
	hr = outputType->SetGUID (MF_MT_SUBTYPE, MFVideoFormat_HEVC);
	stiMFTest ("H265 Encoder: Failed Setting Output Sub Type", stiRESULT_ERROR);
	hr = outputType->SetUINT32 (MF_MT_AVG_BITRATE, m_un32Bitrate);
	stiMFTest ("H265 Encoder: Failed Setting Output AVG Bitrate", stiRESULT_ERROR);
	hr = MFSetAttributeRatio (outputType, MF_MT_FRAME_RATE, m_fFrameRate * 1000, 1000);
	stiMFTest ("H265 Encoder: Failed Setting Output Frame Rate", stiRESULT_ERROR);
	hr = MFSetAttributeSize (outputType, MF_MT_FRAME_SIZE, m_n32VideoWidth, m_n32VideoHeight);
	stiMFTest ("H265 Encoder: Failed Setting Output Frame Size", stiRESULT_ERROR);
	hr = outputType->SetUINT32 (MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	stiMFTest ("H265 Encoder: Failed Setting Interlaced Mode", stiRESULT_ERROR);
	//eAVEncH265VProfile_Main_420_8 == 1
	hr = outputType->SetUINT32 (MF_MT_MPEG2_PROFILE, m_eProfile); //Only Main (1) is supported
	stiMFTest ("H265 Encoder: Failed Setting Profile", stiRESULT_ERROR);
	//eAVEncH265VLevel == 120
	hr = outputType->SetUINT32 (MF_MT_MPEG2_LEVEL, m_unLevel); //We signal a level of 4 or (120)
	stiMFTest ("H265 Encoder: Failed Setting Level", stiRESULT_ERROR);
	hr = MFSetAttributeRatio (outputType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	stiMFTest ("H265 Encoder: Failed Setting Pixel Aspect Ratio", stiRESULT_ERROR);
	hr = m_pEncoder->SetOutputType (0, outputType, NULL);
	stiMFTest ("H265 Encoder: Failed Setting Output Type", stiRESULT_ERROR);
	m_pOutputType = outputType;

	//--------------------Setup Input Type------------------------------------
	flags = 0;
	hr = MFCreateMediaType (&inputType);
	stiMFTest ("H265 Encoder: Failed Creating Input Media Type", stiRESULT_ERROR);
	hr = inputType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Video);
	stiMFTest ("H265 Encoder: Failed Setting Input Major Type", stiRESULT_ERROR);
	hr = inputType->SetGUID (MF_MT_SUBTYPE, MFVideoFormat_IYUV);
	stiMFTest ("H265 Encoder: Failed Setting Input Sub Type", stiRESULT_ERROR);
	hr = MFSetAttributeRatio (inputType, MF_MT_FRAME_RATE, m_fFrameRate * 1000, 1000);
	stiMFTest ("H265 Encoder: Failed Setting Input Frame Rate", stiRESULT_ERROR);
	hr = MFSetAttributeSize (inputType, MF_MT_FRAME_SIZE, m_n32VideoWidth, m_n32VideoHeight);
	stiMFTest ("H265 Encoder: Failed Setting Input Frame Size", stiRESULT_ERROR);

	hr = m_pEncoder->SetInputType (0, inputType, flags);
	stiMFTest ("H265 Encoder: Failed Setting Input Type", stiRESULT_ERROR);

STI_BAIL:
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::AvVideoFrameRateSet
//
//  Description: Sets the current encoding framerate
//
//  Abstract:
// 
//  Returns:
//		none
//		
//
void CstiMF265Encoder::AvVideoFrameRateSet (float fFrameRate)
{
	m_fFrameRate = fFrameRate;
}

//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::AvVideoProfileSet
//
//  Description: Function used to set the current profile / level / constraint
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if values are set
//		
//
stiHResult CstiMF265Encoder::AvVideoProfileSet (EstiVideoProfile eProfile, unsigned int unLevel, unsigned int unConstraints)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_eProfile = eProfile;
	m_unLevel = unLevel;
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::SetPropertyInt
//
//  Description: Helper for Setting ICodecProperty
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if values are set
//		
//
HRESULT CstiMF265Encoder::SetPropertyInt (GUID id, UINT32 value)
{
	HRESULT hResult = S_OK;
	VARIANT var;
	VariantInit (&var);
	var.vt = VT_UI4;
	var.ulVal = value;
	if (m_pCodecAPI)
	{
		return m_pCodecAPI->SetValue (&id, &var);
	}
	return E_FAIL;
}


//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::SetPropertyBool
//
//  Description: Helper for Setting ICodecProperty
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if values are set
//		
//
HRESULT CstiMF265Encoder::SetPropertyBool (GUID id, bool value)
{
	HRESULT hResult = S_OK;
	VARIANT var;
	VariantInit (&var);
	var.vt = VT_BOOL;
	var.boolVal = value;
	if (m_pCodecAPI)
	{
		return m_pCodecAPI->SetValue (&id, &var);
	}
	return E_FAIL;
}

//////////////////////////////////////////////////////////////////////
//; CstiMF265Encoder::AvVideoRequestKeyFrame
//
//  Description: Request a keyframe from the encoder
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if values are set
//		
//
void CstiMF265Encoder::AvVideoRequestKeyFrame ()
{
	SetPropertyInt (CODECAPI_AVEncVideoForceKeyFrame, 1);
}
