//*****************************************************************************
//
// FileName:        CstiMF265Decoder.cpp
//
// Abstract:        Declaration of the CstiMF265Decoder class used to decode
//					H.265 videos with the MediaFoundation 265 decoder.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "CstiMF265Decoder.h"

CstiMF265Decoder::CstiMF265Decoder (IMFTransform *pDecoder) :
m_un32VideoWidth (0),
m_un32VideoHeight (0)
{
	m_pDecoder = pDecoder;
}


CstiMF265Decoder::~CstiMF265Decoder ()
{
	if (m_pDecoder)
	{
		ULONG references = m_pDecoder->Release ();
		assert (references == 0);
		m_pDecoder = NULL;
	}
	
}

stiHResult CstiMF265Decoder::CreateMF265Decoder (IstiVideoDecoder **pVideoDecoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HRESULT hr = S_OK;
	IMFTransform *pDecoder = NULL;      // Pointer to the decoder.

	for (std::vector<IMFActivate *>::const_iterator i = ntouchPC::MediaFoundation.H265Decoders.begin (); i != ntouchPC::MediaFoundation.H265Decoders.end (); i++)
	{
		IMFActivate * iter = (*i);
		hr = iter->ActivateObject (IID_PPV_ARGS (&pDecoder));
		if (SUCCEEDED (hr))
		{
			IMFAttributes * pDecoderAttributes;
			hr = pDecoder->GetAttributes (&pDecoderAttributes);
			pDecoderAttributes->SetUINT32 (CODECAPI_AVLowLatencyMode, 1);
			if (SUCCEEDED (hr))
			{
				break;
			}
			else
			{
				pDecoder->Release ();
				pDecoder = NULL;
			}
		}
		else
		{
			pDecoder == NULL;
		}
	}

	if (pDecoder)
	{
		*pVideoDecoder = new CstiMF265Decoder (pDecoder);
	}
	else
	{
		hResult = stiMAKE_ERROR (stiRESULT_ERROR);//We couldn't find a valid encoder
	}


STI_BAIL:

	return hResult;
}

stiHResult CstiMF265Decoder::AvDecoderInit (EstiVideoCodec esmdVideoCodec)
{
	//----------------------Create Output Buffer-------------------------------------------
	IMFSample *outputSample;
	MFCreateSample (&outputSample);
	IMFMediaBuffer * pMediaBuffer;
	int bufferSize = HEVC_MAX_WIDTH * HEVC_MAX_HEIGHT * 3 / 2;
	uint8_t * pPictureBytes = new uint8_t[bufferSize] ();
	CstiMFBufferWrapper::Create (&pMediaBuffer, pPictureBytes, bufferSize);
	outputSample->AddBuffer (pMediaBuffer);
	m_pOutputSample = outputSample;
	m_pMediaBuffer = pMediaBuffer;
	//-------------------------------------------------------------------------------------

	return MediaTypesConfigure(HEVC_MAX_WIDTH, HEVC_MAX_HEIGHT);
}

stiHResult CstiMF265Decoder::MediaTypesConfigure (uint32_t width, uint32_t height)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HRESULT hr = S_OK;
	IMFMediaType * inputType;
	DWORD flags = 0;
	IMFMediaType * outputType;
	//--------------------Setup Input Type------------------------------------
	hr = MFCreateMediaType (&inputType);
	stiMFTest ("H265 Decoder: Failed Creating Input Media Type", stiRESULT_ERROR);
	hr = inputType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Video);
	stiMFTest ("H265 Decoder: Failed Setting Input Major Type", stiRESULT_ERROR);
	hr = inputType->SetGUID (MF_MT_SUBTYPE, MFVideoFormat_HEVC);
	stiMFTest ("H265 Decoder: Failed Setting Input Sub Type", stiRESULT_ERROR);
	hr = MFSetAttributeSize (inputType, MF_MT_FRAME_SIZE, width, height);
	stiMFTest ("H265 Decoder: Failed Setting Input Frame Size", stiRESULT_ERROR);
	hr = m_pDecoder->SetInputType (0, inputType, flags);
	stiMFTest ("H265 Decoder: Failed Setting Input Type", stiRESULT_ERROR);
	////--------------------Setup Output Type------------------------------------
	hr = MFCreateMediaType (&outputType);
	stiMFTest ("H265 Decoder: Failed Creating Output Media Type", stiRESULT_ERROR);
	hr = outputType->SetGUID (MF_MT_MAJOR_TYPE, MFMediaType_Video);
	stiMFTest ("H265 Decoder: Failed Setting Output Major Type", stiRESULT_ERROR);
	hr = outputType->SetGUID (MF_MT_SUBTYPE, MFVideoFormat_IYUV);
	stiMFTest ("H265 Decoder: Failed Setting Output Sub Type", stiRESULT_ERROR);
	hr = MFSetAttributeSize (outputType, MF_MT_FRAME_SIZE, width, height);
	stiMFTest ("H265 Decoder: Failed Setting Output Frame Size", stiRESULT_ERROR);
	hr = m_pDecoder->SetOutputType (0, outputType, NULL);
	stiMFTest ("H265 Decoder: Failed Setting Output Type", stiRESULT_ERROR);
	m_pOutputType = outputType;

	hr = m_pDecoder->ProcessMessage (MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
	m_un32VideoWidth = width;
	m_un32VideoHeight = height;
STI_BAIL:
	return hResult;
}

stiHResult CstiMF265Decoder::AvDecoderClose ()
{
	m_pDecoder->ProcessMessage (MFT_MESSAGE_NOTIFY_END_STREAMING, 0);

	if (m_pOutputSample) 
	{ 
		ULONG references = m_pOutputSample->Release (); 
		assert (references == 0);
		m_pOutputSample = NULL; 
	}

	if (m_pMediaBuffer) 
	{ 
		ULONG references = m_pMediaBuffer->Release ();
		assert (references == 0);
		m_pMediaBuffer = NULL; 
	}

	return stiRESULT_SUCCESS;
}

stiHResult CstiMF265Decoder::AvDecoderDecode (uint8_t *pBytes, uint32_t un32Len, uint8_t * flags)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	HRESULT hr = S_OK;
	IMFMediaBuffer * inputBuffer = NULL;
	IMFSample *inputSample = NULL;
	MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
	DWORD status;
	IMFAttributes* outputAttributes = NULL;
	UINT32 nDiscontinuity = 0;

	stiTESTCOND (un32Len > 0, stiRESULT_ERROR);
	if (GrabVideoData (pBytes, un32Len))
	{
		*flags |= FLAG_KEYFRAME;
	}
	//----------------Input Sample------------------------
	hr = MFCreateSample (&inputSample);
	stiMFTest ("H265 Decoder: MFCreateSample", stiRESULT_ERROR);
	CstiMFBufferWrapper::Create (&inputBuffer, pBytes, un32Len);
	stiMFTest ("H265 Decoder: CstiMFBufferWrapper::Create", stiRESULT_ERROR);
	hr = inputSample->AddBuffer (inputBuffer);
	stiMFTest ("H265 Decoder: AddBuffer", stiRESULT_ERROR);
	hr = m_pDecoder->ProcessInput (0, inputSample, NULL);
	stiMFTest ("H265 Decoder: Failed Process Input", stiRESULT_ERROR);
	//----------------Output Buffer------------------------
	outputDataBuffer.dwStreamID = 0;
	outputDataBuffer.pSample = m_pOutputSample;
	outputDataBuffer.dwStatus = 0;
	outputDataBuffer.pEvents = NULL;
	hr = m_pDecoder->ProcessOutput (NULL, 1, &outputDataBuffer, &status);
	if (hr == S_OK)
	{
		*flags |= FLAG_FRAME_COMPLETE;
	}
	else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT || FAILED (hr))
	{
		stiMFTest ("H265 Decoder: Failed Process Output", stiRESULT_ERROR);
	}

	m_pOutputSample->QueryInterface<IMFAttributes> (&outputAttributes);
	if (SUCCEEDED (outputAttributes->GetUINT32 (MFSampleExtension_Discontinuity, &nDiscontinuity)))
	{
		if (nDiscontinuity)
		{
			*flags |= FLAG_IFRAME_REQUEST;
		}
		outputAttributes->Release ();
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

bool CstiMF265Decoder::GrabVideoData (uint8_t *pBytes, uint32_t un32Len)
{			
	if (un32Len <= 8)
	{
		return false;
	}

	uint8_t nal_type = 0;
	int index = 0;
	do
	{
		index = FindStartCode (pBytes, un32Len, index);
		nal_type = ((pBytes[index] >> 1) & 0x3F);
	} 
	while (index > 0 && (nal_type == 32 || nal_type == 34 || nal_type == 1));
	if (nal_type == 33)
	{
		int endIndex = FindStartCode (pBytes, un32Len, index);
		if (endIndex < 0)
		{
			endIndex = un32Len;
		}
		CstiBitStreamReader* reader = new CstiBitStreamReader (&pBytes[index], (endIndex - index) * 8);
		reader->skipBits (16);
		uint32_t vps_id = reader->getBits (4);
		uint32_t max_sublayers_minus1 = reader->getBits (3);
		reader->skipBits (1);
		//-----------Parse PTL------------------
		reader->skipBits (2 + 1 + 5 + 32 + 4 + 16 + 16 + 12);
		reader->skipBits (8 + 16);
		//---------------------------------------
		uint32_t general_level_idc = reader->getBits (8); //General Level IDC
		if (max_sublayers_minus1 > 0)
		{
			bool * sub_layer_profile_present_flag = new bool[8];
			bool * sub_layer_level_present_flag = new bool[8];
			for (int i = 0; i < 8; i++)
			{
				sub_layer_profile_present_flag[i] = reader->getBits (1);
				sub_layer_level_present_flag[i] = reader->getBits (1);
			}

			for (int i = 0; i < max_sublayers_minus1; i++)
			{
				if (sub_layer_profile_present_flag[i])
				{
					reader->skipBits (2 + 1 + 5 + 32 + 1 + 1 + 1 + 1 + 43 + 1 + 8);
				}
				if (sub_layer_level_present_flag[i])
				{
					reader->skipBits (8);
				}
			}
		}
		uint32_t sps_id = reader->getGolombU ();
		uint32_t chromaformat = reader->getGolombU ();
		if (chromaformat == 3)
		{
			reader->skipBits (1);
		}

		uint32_t un32VideoWidth = reader->getGolombU ();
		uint32_t un32VideoHeight = reader->getGolombU ();
		if(m_un32VideoWidth != un32VideoWidth ||   m_un32VideoHeight != un32VideoHeight)
		{
			m_pDecoder->ProcessMessage (MFT_MESSAGE_NOTIFY_END_STREAMING, 0);
			MediaTypesConfigure (un32VideoWidth, un32VideoHeight);
		}
		return true;
	}
	return false;
}

int CstiMF265Decoder::FindStartCode (uint8_t *pBytes, uint32_t un32Len, int Index)
{
	for (int i = Index; i < (un32Len - 8); i++)
	{
		if (pBytes[i + 0] == 0x00)
		{
			if (pBytes[i + 1] == 0x00)
			{
				if (pBytes[i + 2] == 0x00)
				{
					if (pBytes[i + 3] == 0x01)
					{
						return i + 4;
					}
				}
				else if (pBytes[i + 2] == 0x01)
				{
					return i + 3;
				}
			}
		}
	}
	return -1;
}

stiHResult CstiMF265Decoder::AvDecoderPictureGet (uint8_t *pBytes, uint32_t un32Len, uint32_t *width, uint32_t *height)
{
	HRESULT hr = S_OK;
	stiHResult hResult = stiRESULT_SUCCESS;
	IMFMediaBuffer * buffer = NULL;
	m_pOutputSample->GetBufferByIndex (0, &buffer);
	uint8_t* pOutBytes;
	DWORD MaxLength;
	DWORD CurrentLength;
	hr = buffer->Lock (&pOutBytes, &MaxLength, &CurrentLength);
	if (SUCCEEDED (hr))
	{
		if (CurrentLength <= un32Len)
		{
			memcpy (pBytes, pOutBytes, m_un32VideoWidth * m_un32VideoHeight * 3 / 2);
			(*width) = m_un32VideoWidth;
			(*height) = m_un32VideoHeight;
		}
		else
		{
			hResult = stiMAKE_ERROR (stiRESULT_BUFFER_TOO_SMALL);
		}
		hr = buffer->Unlock ();
	}
	ULONG referenceCount = buffer->Release ();
	referenceCount = m_pOutputSample->Release ();
	return hResult;
}
