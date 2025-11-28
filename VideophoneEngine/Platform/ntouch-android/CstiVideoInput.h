/*!
* \file CstiVideoInput.h
* \brief NtouchPC Video Input Class
*
*  Copyright (C) 2010-2015 by Sorenson Communications, Inc.  All Rights Reserved
*/
#pragma once

#include "CstiSoftwareInput.h"


// Includes
#include <queue>

//
#include "stiSVX.h"
#include "stiError.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
#include "BaseVideoInput.h"

#include "CstiVideoEncoder.h"
#include "CstiEventQueue.h"

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

class CstiVideoInput : public BaseVideoInput
{
public:

	static bool isMainSupported;
	static bool isExtendedSupported;
	static EstiH264Level h264Level;

	CstiVideoInput();
	virtual ~CstiVideoInput();

	stiHResult VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs) override;
	stiHResult CodecCapabilitiesGet(EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps) override;
	stiHResult KeyFrameRequest () override;
	stiHResult PrivacySet (EstiBool bEnable) override;
	stiHResult PrivacyGet (EstiBool *pbEnable) const override;
	stiHResult VideoRecordStart () override;
	stiHResult VideoRecordStop () override;
	stiHResult VideoRecordSettingsSet (const IstiVideoInput::SstiVideoRecordSettings *pVideoRecordSettings) override;
	stiHResult VideoRecordFrameGet (SstiRecordVideoFrame *pVideoPacket) override;
	stiHResult CallbackRegister(PstiObjectCallback pfnCallback, size_t CallbackParam, size_t CallbackFromId);
	stiHResult CallbackUnregister(PstiObjectCallback pfnCallback, size_t CallbackParam);
	stiHResult Initialize();
	stiHResult Uninitialize();

#ifdef ACT_FRAME_RATE_METHOD
	void EncoderActualFrameRateCalcAndSet();
#endif

	bool AllowsFragmentationUnits();

	virtual stiHResult CreateVideoEncoder(IstiVideoEncoder **pEncoder, EstiVideoCodec eCodec);

	void SendVideoFrame(uint8_t * pBytes, uint32_t nWidth, uint32_t nHeight, EstiColorSpace eColorSpace);

	bool IsRecording();

	bool AllowsFUPackets();

	stiHResult CaptureCapabilitiesChanged ();

	stiHResult TestVideoCodecSet (EstiVideoCodec codec);

private:
	uint32_t m_un32Width;
	uint32_t m_un32Height;
	uint32_t m_unTargetBitrate;
	uint32_t m_unIntraFramerate;
	uint32_t m_unLevel;
	uint32_t m_unConstraints;

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
	stiMUTEX_ID m_FrameDataMutex;
	stiMUTEX_ID m_EncoderMutex;

#ifdef ACT_FRAME_RATE_METHOD
	uint32_t m_un32FrameCount;
	double	m_dStartFrameTime;
	float	m_fActualFrameRateArray[3];
	float	m_fCurrentEncoderRate;
#endif

	std::queue<FrameData*> m_FrameData;
	std::queue<FrameData*> m_FrameDataEmpty;

	unsigned int m_unFrameRate;
//	stiWDOG_ID m_wdDeliverFrameTimer;
	CstiEventQueue m_eventQueue;

	stiHResult UpdateEncoder();
	stiHResult ResetEncoder();

//	int Task();

//	stiDECLARE_EVENT_MAP(CstiVideoInput);
//	stiDECLARE_EVENT_DO_NOW(CstiVideoInput);

	//
	// Event Handlers
	//
	EstiResult EventRecordStart();
	EstiResult EventRecordStop();

};
