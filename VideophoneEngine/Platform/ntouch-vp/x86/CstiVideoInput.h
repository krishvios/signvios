// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

// Includes
//
#include "stiSVX.h"
#include "stiError.h"
#include "CstiOsTaskMQ.h"
#include "CstiEvent.h"
#include "stiEventMap.h"

#include "IVideoInputVP2.h"
#include "CstiGStreamerVideoGraph.h"
#include "IstiVideoOutput.h"	// for definition of SstiImageSettings
#include "IstiPlatform.h"
#include "CstiVideoInputBase.h"

//
// Constants
//

//
// Typedefs
//


//
// Forward Declarations
//
class CstiMonitorTask;

//
// Globals
//

//
// Class Declaration
//

class CstiVideoInput : public CstiVideoInputBase
{
public:

	CstiVideoInput ();
	~CstiVideoInput () override;
	
	CstiVideoInput (const CstiVideoInput &other) = delete;
	CstiVideoInput (CstiVideoInput &&other) = delete;
	CstiVideoInput &operator= (const CstiVideoInput &other) = delete;
	CstiVideoInput &operator= (CstiVideoInput &&other) = delete;
	
	stiHResult Initialize (CstiMonitorTask *monitorTask);
		
	stiHResult BrightnessSet (
		int brightness) override;
		
	stiHResult SaturationSet (
		int saturation) override;

	EstiBool RcuConnectedGet () const override;

	stiHResult ambientLightGet (
			IVideoInputVP2::AmbientLight *ambiance,
			float *rawVal) override
	{
		if (ambiance)
		{
			*ambiance = AmbientLight::HIGH;
		}

		if (rawVal)
		{
			*rawVal = 0.0;
		}

		return stiRESULT_SUCCESS;
	}
	
	stiHResult FocusRangeGet (
		PropertyRange *range) const override;

	stiHResult SingleFocus() override;

	stiHResult FocusPositionSet(int nPosition) override;

	int FocusPositionGet() override;

	void selfViewEnabledSet (bool enabled) override;

	bool RcuAvailableGet () const override;

	stiHResult NoLockAutoExposureWindowSet () override;
	
	stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings) override;

	void videoRecordToFile() override;

protected:

	stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps) override;

	MonitorTaskBase *monitorTaskBaseGet () override;

	CstiGStreamerVideoGraphBase &videoGraphBaseGet () override;
	const CstiGStreamerVideoGraphBase &videoGraphBaseGet () const override;

private:
	
#if APPLICATION == APP_NTOUCH_VP3 || APPLICATION == APP_NTOUCH_VP4
	stiHResult SelfViewWidgetSet (
		void * QQickItem) override;
	
	stiHResult PlaySet (
		bool play) override;
#endif

	void eventEncodeSettingsSet ();

	stiWDOG_ID m_wdFrameTimer{nullptr};
	
	FILE *m_pInputFile264{nullptr};
	FILE *m_pInputFile263{nullptr};
	FILE *m_pInputFileJpg{nullptr};

	stiHResult TimerHandle (
		CstiEvent *poEvent); // Event

	CstiMonitorTask *m_monitorTask {nullptr};
	CstiGStreamerVideoGraph m_gstreamerVideoGraph;
};
