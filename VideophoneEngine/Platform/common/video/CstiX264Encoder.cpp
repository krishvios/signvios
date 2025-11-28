//*****************************************************************************
//
// FileName:       CstiFFMpegEncoder.cpp
//
// Abstract:       Implementation of the CstiFFMpegDecoder class which handles: 
//				   - Opening and decodeing H.264 files
//
//  Sorenson Communications Inc. Confidential. --  Do not distribute
//  Copyright 2012 Sorenson Communications, Inc. -- All rights reserved
//
//
//*****************************************************************************

#include "CstiX264Encoder.h"

#include <math.h>

#if APPLICATION == APP_NTOUCH_MAC
#include <CoreMedia/CoreMedia.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>
#include <VideoToolbox/VideoToolbox.h>

extern "C" {
#include "libavcodec/avcodec.h"
}
#endif

#define DEFAULT_VIDEO_WIDTH 352
#define DEFAULT_VIDEO_HEIGHT 288
#define DEFAULT_VIDEO_BITRATE 256000

#if APPLICATION != APP_MEDIASERVER 
#define CONSTANT_BIT_RATE
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CstiX264Encoder::CstiX264Encoder( )
	:
m_codec(NULL),
m_eProfile(estiH264_BASELINE),
m_unConstraints(0),
m_unLevel(12)
{
//	m_avFrameRate.den = 0;
//	m_avFrameRate.num = 0;
//	m_avTimeBase.den = 0;
//	m_avTimeBase.num = 0;
}

CstiX264Encoder::~CstiX264Encoder()
{
}

//////////////////////////////////////////////////////////////////////
//; CstiX264Encoder::CreateX264Encoder
//
//  Description: Static function used to create a CAvDecoder
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CstiX264Encoder is created.
//		stiRESULT_MEMORY_ALLOC_ERROR if CstiX264Encoder creation failes.
//
stiHResult CstiX264Encoder::CreateVideoEncoder(CstiX264Encoder **pX264Encoder)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	*pX264Encoder = new CstiX264Encoder();
	stiTESTCOND(pX264Encoder != NULL, stiRESULT_MEMORY_ALLOC_ERROR);

STI_BAIL:

	return hResult;
}

stiHResult CstiX264Encoder::AvVideoCompressFrame(uint8_t *pInputFrame, FrameData *pOutputData, EstiColorSpace colorSpace) 
{

	stiHResult hResult = stiRESULT_SUCCESS;
	int numberOfNALUs=0;
	x264_nal_t *NALUs=NULL;
    x264_picture_t inputPicture;

	stiTESTCOND(m_codec, stiRESULT_ERROR ); //"Compress Frame called but m_codec not initialized.\n");
    
    // Prepare the frame to be encoded
    x264_picture_init(&inputPicture);
    inputPicture.i_qpplus1 = 0;
    
	switch (colorSpace)
	{
		case estiCOLOR_SPACE_NV12:
		{
			inputPicture.img.i_csp = X264_CSP_NV12;
			inputPicture.img.i_stride[0] = inputPicture.img.i_stride[1] = m_n32VideoWidth;
			inputPicture.img.plane[0] = pInputFrame;
			inputPicture.img.plane[1] = inputPicture.img.plane[0] + m_n32VideoWidth * m_n32VideoHeight;
			break;
		}
		case estiCOLOR_SPACE_I420:
		{
#if APPLICATION == APP_NTOUCH_MAC
			if (VTPixelTransferSessionCreate != NULL) {
				CVPixelBufferLockBaseAddress((CVPixelBufferRef)pInputFrame, kCVPixelBufferLock_ReadOnly);
				
				inputPicture.img.i_csp = X264_CSP_I420;
				inputPicture.img.i_stride[0] = CVPixelBufferGetBytesPerRowOfPlane((CVPixelBufferRef)pInputFrame, 0);
				inputPicture.img.i_stride[1] = CVPixelBufferGetBytesPerRowOfPlane((CVPixelBufferRef)pInputFrame, 1);
				inputPicture.img.i_stride[2] = CVPixelBufferGetBytesPerRowOfPlane((CVPixelBufferRef)pInputFrame, 2);
				inputPicture.img.plane[0] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane((CVPixelBufferRef)pInputFrame,0);
				inputPicture.img.plane[1] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane((CVPixelBufferRef)pInputFrame,1);
				inputPicture.img.plane[2] = (uint8_t *)CVPixelBufferGetBaseAddressOfPlane((CVPixelBufferRef)pInputFrame,2);
				CVPixelBufferUnlockBaseAddress((CVPixelBufferRef)pInputFrame, 0);
			}
			else {
				inputPicture.img.i_csp = X264_CSP_I420;
				inputPicture.img.i_stride[0] = ((AVPicture *)pInputFrame)->linesize[0];
				inputPicture.img.i_stride[1] = ((AVPicture *)pInputFrame)->linesize[1];
				inputPicture.img.i_stride[2] = ((AVPicture *)pInputFrame)->linesize[2];
				inputPicture.img.plane[0] = ((AVPicture *)pInputFrame)->data[0];
				inputPicture.img.plane[1] = ((AVPicture *)pInputFrame)->data[1];
				inputPicture.img.plane[2] = ((AVPicture *)pInputFrame)->data[2];
			}
#else
			inputPicture.img.i_csp = X264_CSP_I420;
			inputPicture.img.i_stride[0] = m_n32VideoWidth;
			inputPicture.img.i_stride[1] = inputPicture.img.i_stride[2] = m_n32VideoWidth / 2;
			inputPicture.img.plane[0] = pInputFrame;
			inputPicture.img.plane[1] = pInputFrame + m_n32VideoWidth*m_n32VideoHeight;
			inputPicture.img.plane[2] = inputPicture.img.plane[1] + m_n32VideoWidth*m_n32VideoHeight / 4;
#endif
			break;
		}
		default:
		{
			stiTHROW(stiRESULT_ERROR);
			break;
		}
	}

    inputPicture.i_type = m_bRequestKeyframe ? X264_TYPE_IDR : X264_TYPE_AUTO;
	m_bRequestKeyframe = false;
		

    while (numberOfNALUs == 0 && hResult == stiRESULT_SUCCESS) { // workaround for first 2 packets being 0
      x264_picture_t outputPicture;
      if (x264_encoder_encode(m_codec, &NALUs, &numberOfNALUs, &inputPicture, &outputPicture) < 0) {
        hResult = stiRESULT_ERROR;
      }
    }

		if (stiRESULT_SUCCESS==hResult) {
			
			for (int i = 0; i < numberOfNALUs; i++) {
				uint32_t payloadSize = NALUs[i].i_payload-4;
				pOutputData->AddFrameData((uint8_t *)&payloadSize, 4);
				pOutputData->AddFrameData(NALUs[i].p_payload+4, payloadSize);
				if (NALUs[i].i_type == NAL_SLICE_IDR)
				{
					pOutputData->SetKeyFrame();
				}
			}
		}
		
STI_BAIL:		
    
    return hResult;
}


//////////////////////////////////////////////////////////////////////
//; CstiX264Encoder::AvEncoderClose
//
//  Description: Closes the Video file.
//
//  Abstract:
// 
//  Returns:
//		S_OK if file is closed sucessfully.
//		S_FALSE if closing file failes.
//
stiHResult CstiX264Encoder::AvEncoderClose()
{
	stiHResult hResult = stiRESULT_SUCCESS;
  if (m_codec != NULL) {
    x264_encoder_close(m_codec);
    m_codec = NULL;
  }

////	int nAvError = 0;
//
////	nAvError = av_write_trailer(m_pAvFormatContext);
////	stiTESTCOND(nAvError == 0, stiRESULT_MEMORY_ALLOC_ERROR);
//
//	// Clean up.
//    avcodec_close(m_pAvVideoStream->codec);
////	av_free(m_pAvVideoStream);
////	m_pAvVideoStream = NULL;
//
//    av_free(m_pAvFrame);
//	m_pAvFrame = NULL;
//
////	url_fclose(m_pAvFormatContext->pb);
//	av_free(m_pAvFormatContext);
//	m_pAvFormatContext = NULL;

	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvVideoFrameRateSet
//
//  Description: Static function used to create a CAvDecoder
//
//  Abstract:
// 
//  Returns:
//		none
//		
//
void CstiX264Encoder::AvVideoFrameRateSet(float fFrameRate)
{
	// We store the frame rate as fps but the AVCodec stores
	// it as a time based value 1/fps.  So we have to invert
	// the framerate.

	// tmp
	//    m_context.i_timebase_den = 25;
	//    m_context.i_timebase_num = 1;
	//m_context.i_fps_den = 25;
	// max we do is 25
	m_fFrameRate = fFrameRate;
	//stiTrace("New Frame Rate: %d\n", m_context.i_timebase_den);

}

//////////////////////////////////////////////////////////////////////
//; CstiX264Encoder::AvVideoProfileSet
//
//  Description: Function used to set the current profile
//
//  Abstract:
// 
//  Returns:
//		none
//		
//
stiHResult CstiX264Encoder::AvVideoProfileSet(EstiVideoProfile eProfile, unsigned int  unLevel, unsigned int  unConstraints)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	m_eProfile = eProfile;
	m_unLevel = unLevel;
	m_unConstraints = unConstraints;
	return hResult;
}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvInitializeEncoder
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if CAvEncoder is initialized.
//		stiRESULT_ERROR if CAvEncoder initialize failes.
//
stiHResult CstiX264Encoder::AvInitializeEncoder()
{
	stiHResult hResult = stiRESULT_SUCCESS;

	AvUpdateEncoderSettings();
	
	if (m_bMustRestart || m_codec == NULL)
	{
		AvEncoderClose();
		m_codec = x264_encoder_open(&m_context);
		
		if (m_codec == NULL)
			hResult = stiRESULT_ERROR;
		
	}
	else
	{
		x264_encoder_reconfig(m_codec, &m_context);
	}

  return hResult;

}

//////////////////////////////////////////////////////////////////////
//; CstiFFMpegEncoder::AvUpdateEncoderSettings
//
//  Description: 
//
//  Abstract:
// 
//  Returns:
//		stiRESULT_SUCCESS if settings are updated
//		stiRESULT_ERROR if updating of settings fails.
//
stiHResult CstiX264Encoder::AvUpdateEncoderSettings()
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
	
	//X264 allows i_bitrate, i_vbv_max_bitrate and i_vbv_buffer_size to be modified
	//without restarting the encoder as long as VBV was set to on when the encoder was
	//opened (i_vbv_max_bitrate and i_vbv_buffer_size > 0). (See x264, encoder.c,
	//x264_encoder_reconfig() to identify all parameters that can be reconfigured without
	//restarting the encoder.)
	//
	//Important!! All non reconfigurable parameters that may be modified below must
	//be checked in the following if statement to ensure the encoder is restarted
	//if required.
	if (m_context.i_width != m_n32VideoWidth ||
		m_context.i_height != m_n32VideoHeight ||
		m_context.i_fps_num != (uint32_t)(m_fFrameRate * 1000) ||
		m_context.i_keyint_max != (!m_nIFrame ? X264_KEYINT_MAX_INFINITE : m_nIFrame))
	{
		m_bMustRestart = estiTRUE;
	}
	else
	{
		m_bMustRestart = estiFALSE;
	}

	x264_param_default_preset(&m_context, encoderPreset.c_str(), "fastdecode,zerolatency");

#if 1
	switch (m_eProfile)
	{
	case estiH264_MAIN:
		x264_param_apply_profile(&m_context, "main");
		break;
	default:
		x264_param_apply_profile(&m_context, "baseline");
		break;
	}

  // TODO fix to pass level in from VideoSettings
	m_context.i_level_idc = m_unLevel;

	m_context.i_width =  m_n32VideoWidth;
	m_context.i_height = m_n32VideoHeight;
	m_context.i_fps_num = static_cast<uint32_t>(m_fFrameRate * 1000);
	m_context.i_fps_den = 1000;
	
	
	// rate control

	//m_context.rc.i_bitrate = /* m_context.rc.i_vbv_max_bitrate = */ desiredBitrate < 100 ? 100 : desiredBitrate;
	m_context.rc.i_bitrate = desiredBitrate * 8 / 10 < 100 ? 100 : desiredBitrate * 8 / 10; // 80% of vbv maxrate size allows for better quality
	m_context.rc.i_vbv_max_bitrate = desiredBitrate < 130 ? 130 : desiredBitrate;
	// generally, iframes are about 3-5 times that of p-frames.  x264 enforces min bufsize of at least 3 frames
	// but it's been recommended to have a min of 5.  Generally more available buffer means better
	// quality as well.  At 1/3 of 30 fps that should be ~10.  Lower frame rates will be 
	// m_context.rc.i_vbv_buffer_size = desiredBitrate * 0.33f;
	
    m_context.rc.i_vbv_buffer_size = desiredBitrate * 2;   // The larger VBV buffer size allows more flexibility for improved I-Frames, corrects defect 15646
    
	// further, the max avg rate over a period is given by
	// (T + duration) * bitrate / T.
	// T is the time and duration is the duration of the buffer.
	// so if T is 1 second and our buffer duration is size / bitrate or .33
	// then the theoretical max should be 1.33 above the bitrate
	// which is why to lower the bitrate by 20% above.
	
	m_context.i_keyint_max = !m_nIFrame ? X264_KEYINT_MAX_INFINITE : m_nIFrame;
	m_context.i_keyint_min = 30; // default 0
	
	// best quality improvement over stock ultrafast
	m_context.b_deblocking_filter = 1; // 1; default 0 - best quality improvement

	#if defined CONSTANT_BIT_RATE
		//Rate Control
		m_context.rc.i_rc_method = X264_RC_ABR;
		m_context.rc.i_bitrate = desiredBitrate;
		m_context.rc.i_vbv_max_bitrate = m_context.rc.i_bitrate;
		m_context.rc.i_vbv_buffer_size = (int)((float)m_context.rc.i_vbv_max_bitrate * (5000.0 / (float)m_context.i_fps_num)); //buffer equal to 5 frames
	#endif
	
#else

// NOTE
//	m_context.rc.i_vbv_buffer_size = 0; //20000;// NOTE  what to set to?
	//comment: without vbv_buffer_size set, vbv_max_bitrate is ignored (vbv is not enabled), what was wanted here?
	m_bMustRestart = estiTRUE; //If i_vbv_buffer_size is not set > 0, restart is required. Remove this line if buffer size is set to > 0 above.
	m_context.rc.i_bitrate = m_context.rc.i_vbv_max_bitrate = desiredBitrate < 100 ? 100 : desiredBitrate;
	

	m_context.i_width = m_n32VideoWidth;
	m_context.i_height = m_n32VideoHeight;
	m_context.i_fps_num = m_fFrameRate * 1000;
	m_context.i_fps_den = 1000;


// settings affecting speed
// from x264 --fullhelp
// ultrafast:
//	--no-8x8dct --aq-mode 0 --b-adapt 0
//	--bframes 0 --no-cabac --no-deblock
//	--no-mbtree --me dia --no-mixed-refs
//	--partitions none --rc-lookahead 0 --ref 1
//	--scenecut 0 --subme 0 --trellis 0
//	--no-weightb --weightp 0



// tuning
//	- fastdecode:
//		--no-cabac --no-deblock --no-weightb
//		--weightp 0
//	- zerolatency:
//		--bframes 0 --force-cfr --no-mbtree
//		--sync-lookahead 0 --sliced-threads
//		--rc-lookahead 0


	m_context.i_level_idc=12; // default -1


// frame options	
	m_context.i_frame_reference = 1;
	
	m_context.i_keyint_max = 250;
	m_context.i_keyint_min = 25; // default 0
	m_context.b_repeat_headers = 1;

	
	m_context.i_bframe = 0;
	m_context.i_bframe_adaptive = 0;
	m_context.i_scenecut_threshold  = 0; // was 40

	m_context.b_deblocking_filter = 1; // 1; default 0 - best quality improvement
	m_context.i_deblocking_filter_alphac0 = 0;
	m_context.i_deblocking_filter_beta = 0;
	
	m_context.b_cabac=0;


// rate control
		
	m_context.rc.b_stat_write = 0; // NOTE correct?
	m_context.rc.i_rc_method = X264_RC_ABR; // default 1
	m_context.rc.i_qp_min = 10; // 10; // default 0
	m_context.rc.i_qp_max = 51; // 51; // default 69
	m_context.rc.i_qp_step = 4;
	m_context.rc.f_qcompress = 0.6;
	m_context.rc.i_lookahead = 0;
	
	// configure to 0 for 3g bitrate
	m_context.rc.f_rate_tolerance = desiredBitrate <= 100 ? 0 : 1;
	m_context.rc.f_vbv_buffer_init = 0.9;
	m_context.rc.b_mb_tree = 1; // 1; default 0
	m_context.rc.f_ip_factor = 1 / fabs ( 0.71f );
	m_context.rc.f_pb_factor = 1.3f; // from ffmpeg context


// analysis
	m_context.analyse.inter    = 0;
	m_context.analyse.i_direct_mv_pred = 1;
	m_context.analyse.b_weighted_bipred = 0; // NOTE could turn on with higher profile?  
	m_context.analyse.i_weighted_pred = 0;
	m_context.analyse.i_me_method = X264_ME_DIA; // X264_ME_HEX; // default 0
	//m_context.analyse.b_psy = 524288; // NOTE perhaps enabling this might be good
	m_context.analyse.i_me_range = 16; //-1; //16; // default -1
	m_context.analyse.i_subpel_refine = 0;
	m_context.analyse.b_mixed_references = 0;
	m_context.analyse.b_chroma_me = 0; // default 1
	m_context.analyse.b_transform_8x8 = 0;
	m_context.analyse.b_fast_pskip = 1; // default 1
	m_context.analyse.i_trellis = 0;
	m_context.analyse.i_noise_reduction = 0;
	m_context.analyse.b_dct_decimate = 1;

#endif

// threading i/o options
	m_context.i_threads = X264_THREADS_AUTO;
	m_context.i_sync_lookahead = 0;
	m_context.b_sliced_threads = 1;
	
	m_context.b_vfr_input = 0;
	
	// settings from comparing old encoder
	//m_context.vui.b_fullrange = 1;
	
	m_context.b_annexb=false; // default 1
	
	
	m_context.i_slice_max_size = 1300; // default 0
		
  
	return hResult;
}


void CstiX264Encoder::AVEncoderPresetSet(EncoderPresets preset)
{
	//"ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow", "slower", "veryslow", "placebo"
	switch (preset)
	{
	case EncoderPresets::UltraFast:
		encoderPreset = "ultrafast";
		break;
	case EncoderPresets::SuperFast:
		encoderPreset = "superfast";
		break;
	case EncoderPresets::VeryFast:
		encoderPreset = "veryfast";
		break;
	case EncoderPresets::Faster:
		encoderPreset = "faster";
		break;
	case EncoderPresets::Fast:
		encoderPreset = "fast";
		break;
	case EncoderPresets::Medium:
		encoderPreset = "medium";
		break;
	case EncoderPresets::Slow:
		encoderPreset = "slow";
		break;
	case EncoderPresets::Slower:
		encoderPreset = "slower";
		break;
	case EncoderPresets::VerySlow:
		encoderPreset = "veryslow";
		break;
	case EncoderPresets::Placebo:
		encoderPreset = "placebo";
		break;
	default:
		break;
	}

}