/*!
* \file VideoControl.h
* \brief This file containes defines for controlling video
*
* \date Mon Nov 29 2004
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*  All Rights Reserved
*/

#ifndef VIDEO_CONTROL_H
#define VIDEO_CONTROL_H

#include "stiDefs.h"

//
// Use of this file assumes that the size of an int is 32 bits
//

/*!
*  \brief Advanced video settings
*/
typedef struct SstiAdvancedVideoSettings
{
	uint32_t bFocusMode;
	
	uint32_t bFocusProcess;
	
	uint32_t bFocusBoxesDraw;
	
	uint32_t bLSCEnable;			// Lens shading correction
		
	uint32_t bAutoExposureEnable;	// If AE is disabled unExposureTime and unSensorGain are fixed
		
	uint32_t unTargetBrightness;
	
	uint32_t unExposureTime;
	uint32_t unExposureTimeMin;
	uint32_t unExposureTimeMax;
	
	uint32_t unSensorGain;
	uint32_t unSensorGainMin;
	uint32_t unSensorGainMax;

} SstiAdvancedVideoSettings;

typedef struct SstiFloatRGB
{
	float fRed;
	float fGreen;
	float fBlue;
} SstiFloatRGB;

typedef struct SstiRGB
{
	uint32_t un32Red;
	uint32_t un32Green;
	uint32_t un32Blue;
} SstiRGB;

#define FOCUS_STDDEV_AREA_NUMBER 5
#define FOCUS_MAX_AREA_NUMBER 5

typedef struct SstiFocusStatus
{
	uint8_t bValid;

	SstiFloatRGB StdDevArea[FOCUS_STDDEV_AREA_NUMBER];
	
	SstiRGB MaxArea[FOCUS_MAX_AREA_NUMBER];
	
} SstiFocusStatus;

/*!
*  \brief Advanced video status
*/
typedef struct SstiAdvancedVideoStatus
{
	uint32_t unExposureTime;
	
	uint32_t unSensorGain;
	
	uint32_t unUnclippedSensorGain;

	uint32_t unLuminance;
	
	uint32_t unColorTemp;
	
	SstiFocusStatus FocusStatus;

} SstiAdvancedVideoStatus;

/*!
*  \brief Advanced status
*/
struct SstiAdvancedVideoInputStatus
{
	float videoFrameRate;
};

enum EstiH264PacketType
{
	estiH264PACKET_NONE = 0,
	estiH264PACKET_NAL_CODED_SLICE_OF_NON_IDR = 1,
	estiH264PACKET_NAL_CODED_SLICE_PARTITION_A = 2,
	estiH264PACKET_NAL_CODED_SLICE_PARTITION_B = 3,
	estiH264PACKET_NAL_CODED_SLICE_PARTITION_C = 4,
	estiH264PACKET_NAL_CODED_SLICE_OF_IDR = 5,
	estiH264PACKET_NAL_SEI = 6,
	estiH264PACKET_NAL_SPS = 7,
	estiH264PACKET_NAL_PPS = 8,
	estiH264PACKET_NAL_AUD = 9,
	estiH264PACKET_NAL_END_OF_SEQUENCE = 10,
	estiH264PACKET_NAL_END_OF_STREAM = 11,
	estiH264PACKET_NAL_FILLER_DATA = 12,
	estiH264PACKET_NAL_SPS_EXTENSION = 13,
	estiH264PACKET_NAL_PREFIX_NALU = 14,
	estiH264PACKET_NAL_SUBSET_SPS = 15,
	estiH264PACKET_NAL_RESERVED16 = 16,
	estiH264PACKET_NAL_RESERVED17 = 17,
	estiH264PACKET_NAL_RESERVED18 = 18,
	estiH264PACKET_NAL_CODED_SLICE_AUX = 19,
	estiH264PACKET_NAL_CODED_SLICE_EXT = 20,
	estiH264PACKET_NAL_CODED_SLICE_EXT_DEPTH_VIEW = 21,
	estiH264PACKET_NAL_RESERVED22 = 22,
	estiH264PACKET_NAL_RESERVED23 = 23,
	estiH264PACKET_STAPA = 24,
	estiH264PACKET_STAPB = 25,
	estiH264PACKET_MTAP16 = 26,
	estiH264PACKET_MTAP24 = 27,
	estiH264PACKET_FUA = 28,
	estiH264PACKET_FUB = 29,
};

enum EstiH265PacketType
{
	estiH265PACKET_NAL_UNIT_CODED_SLICE_TRAIL_N     = 0,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_TRAIL_R     = 1,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_TSA_N       = 2,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_TLA_R       = 3,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_STSA_N      = 4,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_STSA_R      = 5,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RADL_N      = 6,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RADL_R      = 7,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RASL_N      = 8,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RASL_R      = 9,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RSV_VCL_N10 = 10,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RSV_VCL_N12 = 11,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RSV_VCL_N14 = 12,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RSV_VCL_R11 = 13,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RSV_VCL_R13 = 14,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_RSV_VCL_R15 = 15,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_LP    = 16,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_W_RADL  = 17,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_BLA_N_LP    = 18,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_W_RADL  = 19,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_IDR_N_LP    = 20,
	estiH265PACKET_NAL_UNIT_CODED_SLICE_CRA         = 21,
	estiH265PACKET_NAL_UNIT_RSV_IRAP_VCL22          = 22,
	estiH265PACKET_NAL_UNIT_RSV_IRAP_VCL23          = 23,
	estiH265PACKET_NAL_UNIT_RSV_VCL24               = 24,
	estiH265PACKET_NAL_UNIT_RSV_VCL25               = 25,
	estiH265PACKET_NAL_UNIT_RSV_VCL26               = 26,
	estiH265PACKET_NAL_UNIT_RSV_VCL27               = 27,
	estiH265PACKET_NAL_UNIT_RSV_VCL28               = 28,
	estiH265PACKET_NAL_UNIT_RSV_VCL29               = 29,
	estiH265PACKET_NAL_UNIT_RSV_VCL30               = 30,
	estiH265PACKET_NAL_UNIT_RSV_VCL31               = 31,
	estiH265PACKET_NAL_UNIT_VPS                     = 32,
	estiH265PACKET_NAL_UNIT_SPS                     = 33,
	estiH265PACKET_NAL_UNIT_PPS                     = 34,
	estiH265PACKET_NAL_UNIT_ACCESS_UNIT_DELIMITER   = 35,
	estiH265PACKET_NAL_UNIT_EOS                     = 36,
	estiH265PACKET_NAL_UNIT_EOB                     = 37,
	estiH265PACKET_NAL_UNIT_FILLER_DATA             = 38,
	estiH265PACKET_NAL_UNIT_PREFIX_SEI              = 39,
	estiH265PACKET_NAL_UNIT_SUFFIX_SEI              = 40,
	estiH265PACKET_NAL_UNIT_RSV_NVCL41              = 41,
	estiH265PACKET_NAL_UNIT_RSV_NVCL42              = 42,
	estiH265PACKET_NAL_UNIT_RSV_NVCL43              = 43,
	estiH265PACKET_NAL_UNIT_RSV_NVCL44              = 44,
	estiH265PACKET_NAL_UNIT_RSV_NVCL45              = 45,
	estiH265PACKET_NAL_UNIT_RSV_NVCL46              = 46,
	estiH265PACKET_NAL_UNIT_RSV_NVCL47              = 47,
	estiH265PACKET_AP                               = 48,
	estiH265PACKET_FU                               = 49,
	estiH265PACKET_NAL_UNIT_INVALID                 = 64,
};


#endif //#ifndef VIDEO_CONTROL_H
