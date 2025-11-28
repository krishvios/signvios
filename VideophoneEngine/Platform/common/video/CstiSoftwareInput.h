/*!
* \file CstiSoftwareInput.h
* \brief Platform layer Software VideoInput Abstract Class
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/


#ifndef CSTISOFTWAREVIDEOINPUT_H
#define CSTISOFTWAREVIDEOINPUT_H

// Includes

#include <queue>

//
#include "stiSVX.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"

#include "IstiVideoInput.h"
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

class CstiSoftwareInput : public CstiOsTaskMQ, public IstiVideoInput
{
public:

	enum EEventType 
	{
		estiSOFTWARE_INPUT_RECORD_START = 256,
		estiSOFTWARE_INPUT_RECORD_STOP,
		estiSOFTWARE_INPUT_SHUTDOWN,
	};

	CstiSoftwareInput ();
	virtual ~CstiSoftwareInput ();

	virtual uint32_t BrightnessDefaultGet () const;
		
	virtual stiHResult BrightnessRangeGet (
		uint32_t *pun32Min,
		uint32_t *pun32Max) const;
		
	virtual stiHResult BrightnessSet (
		uint32_t un32Brightness);

	virtual stiHResult CameraSet (
		EstiSwitch eSwitch);

	stiHResult CaptureCapabilitiesChanged ();

	stiHResult Initialize ();
	stiHResult Uninitialize ();
	
#ifdef ACT_FRAME_RATE_METHOD
	void EncoderActualFrameRateCalcAndSet();
#endif
	
	virtual stiHResult KeyFrameRequest ();

	virtual stiHResult PrivacySet (
		EstiBool bEnable);

	virtual stiHResult PrivacyGet (
		EstiBool *pbEnable) const;

	virtual stiHResult SaturationRangeGet (
		uint32_t *pun32Min,
		uint32_t *pun32Max) const;
		
	virtual uint32_t SaturationDefaultGet () const;
		
	virtual stiHResult SaturationSet (
		uint32_t un32Saturation);

	virtual stiHResult VideoRecordStart ();

	virtual stiHResult VideoRecordStop ();

	virtual stiHResult VideoRecordCodecSet (
		EstiVideoCodec eVideoCodec);
	
	virtual stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings);

	virtual stiHResult VideoRecordFrameGet (
		SstiRecordVideoFrame *pVideoPacket);
    
    virtual bool FrameAvailableGet();
    
    virtual uint8_t* AllocateInputBuffer(
        const uint32_t yuvSize);
    
    virtual void DeleteInputBuffer(
        uint8_t* buffer);
    
    virtual stiHResult CreateVideoEncoder(
        IstiVideoEncoder **pEncoder,
        EstiVideoCodec eCodec);

	bool AllowsFragmentationUnits();

protected:
		
	virtual void CaptureEnabledSet (EstiBool bEnabled)=0;
#if APPLICATION == APP_NTOUCH_PC || APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
	virtual EstiBool VideoInputRead ( uint8_t **pBytes, EstiColorSpace * pColorSpace)=0;
#else
	virtual EstiBool VideoInputRead ( uint8_t *pBytes)=0;
#endif
	
	virtual void VideoRecordSizeSet(uint32_t un32Width, uint32_t un32Height)=0;
	virtual void VideoRecordFrameRateSet ( uint32_t un32FrameRate )=0;
    
	virtual EstiBool ThreadDeliverFrame ();


private:
	SstiVideoRecordSettings m_stCurrentSettings;
	
	EstiBool m_bFirstFrame;
	EstiBool m_bInitialized;
	EstiBool m_bKeyFrame;
	EstiBool m_bPrivacy;
	EstiBool m_bRecording;
	EstiBool m_bShutdownMsgReceived;
	EstiVideoCodec m_eVideoCodec;
	IstiVideoEncoder *m_pVideoEncoder;
	stiMUTEX_ID m_FrameDataMutex;
	stiMUTEX_ID m_EncoderMutex;
	uint8_t* m_pInputBuffer;
	uint32_t m_unInputBufferSize;
	uint32_t m_discardedFrameCount;

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

	stiHResult SetupEncoder();

	//
	// Member functions
	//
	int Task () override;


	stiDECLARE_EVENT_MAP (CstiSoftwareInput);
	stiDECLARE_EVENT_DO_NOW (CstiSoftwareInput);

	//
	// Event Handlers
	//
	
	EstiResult stiEVENTCALL EventRecordStart (
		IN void* poEvent);
	EstiResult stiEVENTCALL EventRecordStop (
		IN void* poEvent);
	stiHResult stiEVENTCALL ShutdownHandle (
		IN void* poEvent);

};

#endif //#ifndef CstiSoftwareInput_H
