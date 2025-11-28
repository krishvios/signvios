// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTI_FILE_VIDEO_INPUT_H
#define CSTI_FILE_VIDEO_INPUT_H

// Includes
//
#include "stiSVX.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"
#include "BaseVideoInput.h"

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

class CstiFileVideoInput : public CstiOsTaskMQ, public BaseVideoInput
{
public:

	enum EEventType 
	{
		estiVIDEO_INPUT_FRAME_CAPTURE_REQUEST = 256,
		estiVIDEO_INPUT_FRAME_CAPTURE_RECEIVED,
		estiVIDEO_INPUT_DISPLAY_SETTINGS_SET,
		estiVIDEO_INPUT_ENCODE_SETTINGS_SET,
	};

	CstiFileVideoInput ();
	
	virtual ~CstiFileVideoInput ();
	
	stiHResult Initialize ();
	
	stiHResult Uninitialize ();
	virtual stiHResult VideoRecordStart ();

	virtual stiHResult VideoRecordStop ();
	virtual stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings);

	virtual stiHResult VideoRecordFrameGet (
		SstiRecordVideoFrame * pVideoFrame);
	virtual stiHResult PrivacySet (
		EstiBool bEnable);

	virtual stiHResult PrivacyGet (
		EstiBool *pbEnable) const;

	virtual stiHResult KeyFrameRequest ();

protected:

virtual stiHResult VideoCodecsGet (
	std::list<EstiVideoCodec> *pCodecs);

virtual stiHResult CodecCapabilitiesGet (
	EstiVideoCodec eCodec,
	SstiVideoCapabilities *pstCaps);

private:
	
	EstiBool m_bPrivacy;
	
	stiWDOG_ID m_wdFrameTimer;
	
	EstiVideoCodec m_eVideoCodec;

	FILE *m_pInputFile264;
	FILE *m_pInputFile263;
	FILE *m_pInputFileJpg;

	bool m_bVideoRecordStarted;

	int Task () override;

	stiDECLARE_EVENT_MAP (CstiFileVideoInput);
	stiDECLARE_EVENT_DO_NOW (CstiFileVideoInput);
	
	stiHResult TimerHandle (
		CstiEvent *poEvent); // Event

	stiHResult ShutdownHandle (
		CstiEvent* poEvent);  // The event to be handled
};

#endif //#ifndef CSTI_FILE_VIDEO_INPUT_H
