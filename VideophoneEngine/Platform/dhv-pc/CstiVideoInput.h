/*!
* \file CstiVideoInput.h
* \brief NtouchPC Video Input Class
*
*  Copyright (C) 2010-2015 by Sorenson Communications, Inc.  All Rights Reserved
*/
#pragma once

// Includes
#include <queue>

//
#include "stiSVX.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"

#include "BaseVideoInput.h"
#include "CstiVideoEncoder.h"
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

class CstiVideoInput : public CstiOsTaskMQ, public BaseVideoInput
{
	public:
	
	enum EEventType
	{
		estiVIDEO_INPUT_RECORD_START = 256,
		estiVIDEO_INPUT_RECORD_STOP,
		estiVIDEO_INPUT_PACKET_SIGNAL,
		estiVIDEO_INPUT_SHUTDOWN,
	};
		
	CstiVideoInput();
	virtual ~CstiVideoInput();
	
	stiHResult VideoCodecsGet(std::list<EstiVideoCodec> *pCodecs) override;
	stiHResult CodecCapabilitiesGet(EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps) override;
	stiHResult KeyFrameRequest () override;
	stiHResult PrivacySet (EstiBool bEnable) override;
	stiHResult PrivacyGet (EstiBool *pbEnable) const override;
	stiHResult VideoRecordStart () override;
	stiHResult VideoRecordStop () override;
	stiHResult VideoRecordCodecSet (EstiVideoCodec eVideoCodec) override;
	stiHResult VideoRecordSettingsSet (const SstiVideoRecordSettings *pVideoRecordSettings) override;
	stiHResult VideoRecordFrameGet (SstiRecordVideoFrame *pVideoPacket) override;

#ifdef ACT_FRAME_RATE_METHOD
	void EncoderActualFrameRateCalcAndSet();
#endif

	void SendVideoFrame(UINT8* pBytes, UINT32 nWidth, UINT32 nHeight, EstiColorSpace eColorSpace);

	void SendVideoFrame(UINT8* pBytes, INT32 nDataLength, UINT32 nWidth, UINT32 nHeight, EstiVideoCodec eCodec);

	bool IsRecording();

	bool AllowsFUPackets();

	stiHResult CaptureCapabilitiesChanged ();

	stiHResult TestVideoCodecSet (EstiVideoCodec codec);

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
	stiWDOG_ID m_wdDeliverFrameTimer;

	void UpdateEncoder();

	int Task();

	stiDECLARE_EVENT_MAP(CstiVideoInput);
	stiDECLARE_EVENT_DO_NOW(CstiVideoInput);

	//
	// Event Handlers
	//
	EstiResult stiEVENTCALL EventRecordStart(void* poEvent);
	EstiResult stiEVENTCALL EventRecordStop(void* poEvent);
	stiHResult stiEVENTCALL ShutdownHandle(void* poEvent);

};
