// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

//
// Includes
//

// __STDC_CONSTANT_MACROS must be defined so that UINT64_C will be defined in stdint.h
#ifdef __cplusplus
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#endif

#include "stiSVX.h"
#include "stiError.h"

#include "CstiVideoInputBase.h"
#include "CstiGStreamerVideoGraph.h"
#include "GStreamerSample.h"

#include "CstiISPMonitor.h"

#include "IstiVideoOutput.h"	// for definition of SstiImageSettings

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/v4l2-mediabus.h>
#include <media/ov640_v4l2.h>
#include <cstdint>
#include <fcntl.h>
#include "stiOVUtils.h"
#include <memory>
#include <mutex>


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

	stiHResult Initialize (
		CstiMonitorTask *pMonitorTask);

	// Video

	stiHResult FocusRangeGet(
		PropertyRange *range) const override;

	stiHResult BrightnessSet (
		int brightness) override;  // nolock

	stiHResult SaturationSet (
		int saturation) override;

	stiHResult SingleFocus () override;  // async

	int FocusPositionGet () override;

	stiHResult FocusWindowGet (
		int *pnStartX,
		int *pnStartY,
		int *pnWinW,
		int *pnWinH);

	stiHResult FocusPositionSet (
		int nFocusValue) override;  // async

	virtual stiHResult HdmiPassthroughSet (
		bool bHdmiPassthrough);  // async

	bool RcuAvailableGet () const override;

	void SetISPMonitor (CstiISPMonitor *pMonitor)
	{
		Lock();
		m_pISPMonitor = pMonitor;
		Unlock();
	}

	stiHResult ambientLightGet (
		IVideoInputVP2::AmbientLight *ambiance,
		float *rawVal) override;
		
	stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings) override;

protected:
	
	void eventEncodeSettingsSet (std::shared_ptr<SstiVideoRecordSettings> videoRecordSettings);

	stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps) override;


	void videoRecordToFile() override;

	MonitorTaskBase *monitorTaskBaseGet() override;

	CstiGStreamerVideoGraphBase &videoGraphBaseGet () override;
	const CstiGStreamerVideoGraphBase &videoGraphBaseGet () const override;

private:
	//
	// Member functions
	//

	//
	// Event Handlers (run from event queue)
	//
	void EventHdmiInResolutionChanged(int resolution);
	void EventHdmiPassthroughSet (bool bHdmiPassthrough);
	void EventAutoFocusTimerTimeout();
	void EventSingleFocus ();
	void EventFocusPositionSet (int nFocusValue);
	void EventPlayingValidate ();

	stiHResult NoLockCameraOpen () override;
	stiHResult NoLockCameraClose () override;
	stiHResult NoLockRcuStatusChanged () override;
	stiHResult NoLockCameraSettingsSet () override;

	stiHResult NoLockCachedSettingSet ();

	void NoLockWindowScale (
		int nOriginalWidth,
		int nOriginalHeight,
		int *pnX1,
		int *pnY1,
		int *pnX2,
		int *pnY2);

	stiHResult NoLockFocusWindowSet (
		uint16_t startX,
		uint16_t startY,
		uint16_t winW,
		uint16_t winH);

	stiHResult NoLockAutoExposureWindowSet () override;

	stiHResult NoLockOv640RegisterSet (
		uint32_t reg,
		uint8_t regVal);

	stiHResult NoLockOv640RegisterGet (
		uint32_t un32Register,
		uint8_t *pun8RegValue);

	stiHResult NoLockPlayingValidateStart ();

	static gboolean AsyncPlayingValidate (
		gpointer pUserData);

private:
	static constexpr const int MIN_FOCUS_VALUE = 0x1;
	static constexpr const int MAX_FOCUS_VALUE = 0x3fe;

private:

	// Check every quarter second for completion of focus
	const int AUTO_FOCUS_WAIT_TIME_MS = 250;

	CstiTimer m_autoFocusTimer {AUTO_FOCUS_WAIT_TIME_MS};

	int m_nCameraControlFd = -1;

	bool m_bHdmiPassthrough = false;

	int m_nFocusValue = MIN_FOCUS_VALUE;
	bool m_bCachedFocusValue = false;

	CstiISPMonitor *m_pISPMonitor = nullptr;

	CstiMonitorTask *m_monitorTask{nullptr};
	CstiGStreamerVideoGraph m_gstreamerVideoGraph;
};
