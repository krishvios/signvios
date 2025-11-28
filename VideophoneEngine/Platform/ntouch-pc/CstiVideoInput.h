/*!
* \file CstiVideoInput.h
* \brief NtouchPC Video Input Class
*
*  Copyright (C) 2010-2015 by Sorenson Communications, Inc.  All Rights Reserved
*/
#pragma once

#include "CstiNativeVideoLink.h"


// Includes
#include <queue>

//
#include "stiSVX.h"
#include "stiError.h"
#include "CstiEventQueue.h"

#include "BaseVideoInput.h"
#include "IPlatformVideoInput.h"
#include "CstiVideoEncoder.h"

#include "PropertyManager.h"
//
// Constants
//

//
// Typedefs
//


//
// Forward Declarations
//


//
// Globals
//

//
// Class Declaration
//

class CstiVideoInput : public CstiEventQueue, public BaseVideoInput, public IPlatformVideoInput
{
public:
		
	CstiVideoInput();
	~CstiVideoInput();
	
	stiHResult VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs) override;
	stiHResult CodecCapabilitiesGet(EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps) override;
	stiHResult KeyFrameRequest () override;
	stiHResult PrivacySet (EstiBool bEnable) override;
	stiHResult PrivacyGet (EstiBool *pbEnable) const override;
	stiHResult VideoRecordStart () override;
	stiHResult VideoRecordStop () override;
	stiHResult VideoRecordSettingsSet (const SstiVideoRecordSettings *pVideoRecordSettings) override;
	stiHResult VideoRecordFrameGet (SstiRecordVideoFrame *pVideoPacket) override;

#ifdef ACT_FRAME_RATE_METHOD
	void EncoderActualFrameRateCalcAndSet();
#endif

	void SendVideoFrame(UINT8* pBytes, UINT32 nWidth, UINT32 nHeight, EstiColorSpace eColorSpace) override;

	void SendVideoFrame(UINT8* pBytes, INT32 nDataLength, UINT32 nWidth, UINT32 nHeight, EstiVideoCodec eCodec) override;

	bool IsRecording() override;

	bool AllowsFUPackets() override;

	stiHResult CaptureCapabilitiesChanged () override;

	stiHResult testCodecSet (EstiVideoCodec codec) override;

private:
	UINT32 m_un32Width;
	UINT32 m_un32Height;
	UINT32 m_unTargetBitrate;
	UINT32 m_unIntraFramerate;
	UINT32 m_unLevel;
	UINT32 m_unConstraints;

	EstiPacketizationScheme m_ePacketization;
	EstiVideoProfile m_eProfile;
	EstiVideoCodec m_eTestVideoCodec;

	bool m_bH265Enabled = true;
	bool m_bInitialized;
	bool m_bKeyFrame;
	bool m_bPrivacy;
	bool m_bRecording;
	bool m_bShutdownMsgReceived;
	EstiVideoCodec m_eVideoCodec;
	IstiVideoEncoder *m_pVideoEncoder;
	std::recursive_mutex m_FrameDataMutex{};
	std::recursive_mutex m_EncoderMutex{};
	
#ifdef ACT_FRAME_RATE_METHOD
	uint32_t m_un32FrameCount;
	double	m_dStartFrameTime;
	float	m_fActualFrameRateArray[3];
	float	m_fCurrentEncoderRate;
#endif

	std::queue<FrameData*> m_FrameData;
	std::queue<FrameData*> m_FrameDataEmpty;

	unsigned int m_unFrameRate;
	stiWDOG_ID m_wdDeliverFrameTimer;

	void UpdateEncoder();

	//
	// Event Handlers
	//
	void eventRecordStart();
	void eventRecordStop();

};
