//*****************************************************************************
//
// FileName:        CstiX265Encoder.h
//
// Abstract:        Declaration of the CstiX265Encoder class used to encode
//					H.265 videos with the x265 encoder.
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//*****************************************************************************

#include "CstiX265Encoder.h"

#include <math.h>

#include "SMCommon.h"

#if APPLICATION == APP_NTOUCH_MAC
#include <CoreMedia/CoreMedia.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>
#include <VideoToolbox/VideoToolbox.h>

extern "C" {
#include "libavcodec/avcodec.h"
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CstiX265Encoder::CstiX265Encoder( )
	:
m_codec(NULL),
m_bMustRestart (estiTRUE)
{
//	m_avFrameRate.den = 0;
//	m_avFrameRate.num = 0;
//	m_avTimeBase.den = 0;
//	m_avTimeBase.num = 0;
	m_context = x265_param_alloc();
}

CstiX265Encoder::~CstiX265Encoder()
{
	x265_param_free(m_context);
	m_context = NULL;
}

//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::CreateX265Encoder
//
//  Description: Static function used to create a CstiX265Encoder
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CstiX265Encoder is created.
//		stiRESULT_MEMORY_ALLOC_ERROR if CstiX265Encoder creation fails.
//
stiHResult CstiX265Encoder::CreateVideoEncoder(CstiX265Encoder **pX265Encoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pX265Encoder = new CstiX265Encoder();
	stiTESTCOND(pX265Encoder != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

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
stiHResult CstiX265Encoder::AvVideoCompressFrame(uint8_t *pInputFrame, FrameData *pOutputData, EstiColorSpace colorSpace)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	uint32_t numberOfNALUs=0;
	
	x265_nal *NALUs=NULL;
	x265_picture outputPicture;
    x265_picture inputPicture;

	int status;
	stiTESTCOND(m_codec, stiRESULT_ERROR ); //"Compress Frame called but m_codec not initialized.\n");
	
    // Prepare the frame to be encoded
	//x265_picture_init(m_context, &inputPicture);
	x265_picture_init(m_context, &inputPicture);
    //inputPicture.//i_qpplus1 = 0;
    
	switch (colorSpace)
	{
		case estiCOLOR_SPACE_NV12:
		{
			inputPicture.colorSpace = X265_CSP_NV12;
			inputPicture.stride[0] = inputPicture.stride[1] = m_n32VideoWidth;
			inputPicture.planes[0] = pInputFrame;
			inputPicture.planes[1] = pInputFrame + m_n32VideoWidth * m_n32VideoHeight;
			break;
		}
		case estiCOLOR_SPACE_I420:
		{
#if APPLICATION == APP_NTOUCH_MAC
			if (VTPixelTransferSessionCreate != NULL) {
				CVPixelBufferLockBaseAddress((CVPixelBufferRef)pInputFrame, kCVPixelBufferLock_ReadOnly);
				
				inputPicture.colorSpace = X265_CSP_I420;
				inputPicture.stride[0] = CVPixelBufferGetBytesPerRowOfPlane((CVPixelBufferRef)pInputFrame, 0);
				inputPicture.stride[1] = CVPixelBufferGetBytesPerRowOfPlane((CVPixelBufferRef)pInputFrame, 1);
				inputPicture.stride[2] = CVPixelBufferGetBytesPerRowOfPlane((CVPixelBufferRef)pInputFrame, 2);
				inputPicture.planes[0] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane((CVPixelBufferRef)pInputFrame,0);
				inputPicture.planes[1] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane((CVPixelBufferRef)pInputFrame,1);
				inputPicture.planes[2] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane((CVPixelBufferRef)pInputFrame,2);
				CVPixelBufferUnlockBaseAddress((CVPixelBufferRef)pInputFrame, 0);
			}
			else {
				inputPicture.colorSpace = X265_CSP_I420;
				inputPicture.stride[0] = ((AVPicture *)pInputFrame)->linesize[0];
				inputPicture.stride[1] = ((AVPicture *)pInputFrame)->linesize[1];
				inputPicture.stride[2] = ((AVPicture *)pInputFrame)->linesize[2];
				inputPicture.planes[0] = ((AVPicture *)pInputFrame)->data[0];
				inputPicture.planes[1] = ((AVPicture *)pInputFrame)->data[1];
				inputPicture.planes[2] = ((AVPicture *)pInputFrame)->data[2];
			}
#else
			inputPicture.colorSpace = X265_CSP_I420;
			inputPicture.stride[0] = m_n32VideoWidth;
			inputPicture.stride[1] = inputPicture.stride[2] = m_n32VideoWidth / 2;
			inputPicture.planes[0] = pInputFrame;
			inputPicture.planes[1] = pInputFrame + m_n32VideoWidth*m_n32VideoHeight;
			inputPicture.planes[2] = (pInputFrame + m_n32VideoWidth*m_n32VideoHeight) + m_n32VideoWidth*m_n32VideoHeight / 4;
#endif
			break;
		}
		default:
		{
			stiTHROW(stiRESULT_ERROR);
			break;
		}
	}
	inputPicture.sliceType = m_bRequestKeyframe ? X265_TYPE_IDR : X265_TYPE_AUTO;
	m_bRequestKeyframe = false;

	status = x265_encoder_encode(m_codec, &NALUs, &numberOfNALUs, &inputPicture, &outputPicture);
	if (status == 1)
	{
		if (IS_X265_TYPE_I(outputPicture.sliceType))
		{
			pOutputData->SetKeyFrame();
		}

		for (size_t i = 0; i < numberOfNALUs; i++)
		{
			//pOutputData->AddFrameData(NALUs[i].payload, NALUs[i].sizeBytes);
			int nHeaderSize = NALUs[i].payload[2] == 0x01 ? 3: 4;
			uint32_t payloadSize = NALUs[i].sizeBytes - nHeaderSize;
			pOutputData->AddFrameData((uint8_t *)&payloadSize, 4);
			pOutputData->AddFrameData(NALUs[i].payload + nHeaderSize, payloadSize);
		}
	}
	else
	{
		hResult = stiMAKE_ERROR(stiRESULT_ERROR);
		stiTrace("Frame not compressed");
	}

STI_BAIL:

    return hResult;
}


//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::AvEncoderClose
//
//  Description: Closes the Video file.
//
//  Abstract:
// 
//  Returns:
//		S_OK if file is closed sucessfully.
//		S_FALSE if closing file failes.
//
stiHResult CstiX265Encoder::AvEncoderClose()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	if (m_codec) 
	{
		FlushEncoder();
		x265_encoder_close(m_codec);
		m_codec = NULL;
	}
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::AvVideoFrameRateSet
//
//  Description: Sets the current encoding framerate
//
//  Abstract:
// 
//  Returns:
//		none
//		
//
void CstiX265Encoder::AvVideoFrameRateSet(float fFrameRate)
{
	m_fFrameRate = fFrameRate;
}


//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::FlushEncoder
//
//  Description: Function Used to Flush the Encoder of Frames
//
//  Abstract:
// 
//  Returns:
//		none
//		
//
void CstiX265Encoder::FlushEncoder()
{
	if (m_codec)
	{
		x265_nal *NALUs = NULL;
		uint32_t numberOfNALUs = 0;
		while (x265_encoder_encode(m_codec, &NALUs, &numberOfNALUs, NULL, NULL) != 0)
		{
			//Flush
		}
	}
}

//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::AvInitializeEncoder
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if x265 is initialized.
//		stiRESULT_ERROR if x265 initialize fails.
//
stiHResult CstiX265Encoder::AvInitializeEncoder()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	AvUpdateEncoderSettings();  

	if (m_bMustRestart || m_codec == NULL)
	{
		AvEncoderClose ();
		m_codec = x265_encoder_open (m_context);
	}
	else
	{
		x265_encoder_reconfig (m_codec, m_context);
	}

	if (m_codec == NULL)
	{
		hResult = stiRESULT_ERROR;
	}
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::AvVideoProfileSet
//
//  Description: Function used to set the current profile / level / constraint
//
//  Abstract:
// 
//  Returns:
//		none
//		
//
stiHResult CstiX265Encoder::AvVideoProfileSet(EstiVideoProfile eProfile, unsigned int  unLevel, unsigned int  unConstraints)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_eProfile = eProfile;
	m_unLevel = unLevel;
	m_unConstraints = unConstraints;
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiX265Encoder::AvUpdateEncoderSettings
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if settings are updated
//		stiRESULT_ERROR if updating of settings fails.
//
stiHResult CstiX265Encoder::AvUpdateEncoderSettings()
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	uint32_t desiredBitrate;

#ifdef ACT_FRAME_RATE_METHOD
	/*CstiSoftwareInput::EncoderActualFrameRateCalcAndSet calculates the actual frame rate being delivered
	*by the camera and sets the CstiVideoEncoder member variable m_fActualFrameRate. Because the encoder
	*is not time dependent, a ratio of the target frame rate and the actual frame rate multiplied by the
	*requested bitrate corrects the delivered bitrate when the camera is not providing the target frame
	*rate. This is beneficial because modifying the bitrate requested from the encoder allows the encoder
	*to be reconfigured and not restarted. Conversely, if the encoder frame rate were to be altered to
	*match the camera frame rate, the encoder would have to be restarted for the change to take affect.
	*This method is not required for endpoints with fixed camera rates, ie. VP.
	*/
	if ((int)m_fActualFrameRate)
	{
		desiredBitrate = (uint32_t)((float)m_un32Bitrate * m_fFrameRate / m_fActualFrameRate / 1000.0);
	}
	else
#endif
	{
		desiredBitrate = m_un32Bitrate / 1000;
	}

	//X265 allows some paramters to be modified without restarting the encoder as long as VBV was set to
	//on when the encoder was opened (i_vbv_max_bitrate and i_vbv_buffer_size > 0).
	//(See x265_encoder_reconfig() to identify all parameters that can be reconfigured without restarting the encoder.)
	//
	//Important!! All non reconfigurable parameters that may be modified below must
	//be checked in the following if statement to ensure the encoder is restarted
	//if required.
	if (m_context->sourceWidth != m_n32VideoWidth ||
		m_context->sourceHeight != m_n32VideoHeight ||
		m_context->fpsNum != (uint32_t)(m_fFrameRate * 1000))
	{
		m_bMustRestart = estiTRUE;
	}
	else
	{
		m_bMustRestart = estiFALSE;
	}

	int success;
	//Fastest preset is always the first according to 265 documentation.
	//x265_preset_names[] = { "ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo", 0 };
	//Basic profile x265_profile_names[] = { "main", "main10", "mainstillpicture", 0 };
	success = x265_param_default_preset(m_context, "ultrafast", "zerolatency"); //fastdecode,
	success = x265_param_apply_profile (m_context, "main");
	
	//------------------------------Fast Decode
	m_context->bEnableLoopFilter = 0;
	m_context->bEnableSAO = 0;
	m_context->bEnableWeightedPred = 0;
	m_context->bEnableWeightedBiPred = 0;
	m_context->bIntraInBFrames = 0;
	//-------------------------------Zero Latency
	m_context->bFrameAdaptive = 0;
	m_context->bframes = 0;
	m_context->lookaheadDepth = 1;
	m_context->scenecutThreshold = 0;
	m_context->rc.cuTree = 0;
	m_context->bEnableEarlySkip = false;
	m_context->bEnableFastIntra = false;
	//------------------------------------------
	//m_context->searchMethod = X265_ME_METHODS::X265_DIA_SEARCH;
	m_context->sourceWidth =  m_n32VideoWidth;
	m_context->sourceHeight = m_n32VideoHeight;
	m_context->fpsNum = static_cast<uint32_t>(m_fFrameRate * 1000);
	m_context->fpsDenom = 1000;
	m_context->internalCsp = X265_CSP_I420;
	m_context->bRepeatHeaders = true;
	//------------------------------------------
	m_context->frameNumThreads = 0;
	// rate control
	m_context->rc.rateControlMode = X265_RC_ABR;
	m_context->rc.bitrate = desiredBitrate < 100 ? 100 : desiredBitrate; // 80% of vbv maxrate size allows for better quality
	//m_context->rc.vbvMaxBitrate = desiredBitrate < 130 ? 130 : desiredBitrate;
    //m_context->rc.vbvBufferSize = desiredBitrate * 2;   // The larger VBV buffer size allows more flexibility for improved I-Frames, corrects defect 15646
	m_context->keyframeMax = -1;
	m_context->keyframeMin = 30;
	// further, the max avg rate over a period is given by
	// (T + duration) * bitrate / T.
	// T is the time and duration is the duration of the buffer.
	// so if T is 1 second and our buffer duration is size / bitrate or .33
	// then the theoretical max should be 1.33 above the bitrate
	// which is why to lower the bitrate by 20% above.
	
	//m_context->keyframeMax = 120;
	//m_context->keyframeMin = 30; // default 0
	
	// settings from comparing old encoder
	
	//m_context->maxCUSize = 256; // default 0
		
  
	return hResult;
}


