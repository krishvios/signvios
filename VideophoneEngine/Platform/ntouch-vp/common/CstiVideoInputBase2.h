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
#include "IstiVideoOutput.h"	// for definition of SstiImageSettings

#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <chrono>

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

class CstiVideoInputBase2 : public CstiVideoInputBase
{
public:
	CstiVideoInputBase2 ();

	~CstiVideoInputBase2 () override;

	stiHResult Initialize (
		CstiMonitorTask *pMonitorTask);

	// Video
	stiHResult BrightnessRangeGet (
		PropertyRange *range) const override;

	int BrightnessGet () const override;

	stiHResult BrightnessSet (
		int brightness) override;

	stiHResult SaturationRangeGet (
		PropertyRange *range) const override;

	int SaturationGet () const override;

	stiHResult SaturationSet (
		int saturation) override;

	stiHResult FocusRangeGet (
		PropertyRange *range) const override;
		
	int FocusPositionGet () override;

	stiHResult FocusPositionSet (
		int focusPosition) override;

	stiHResult contrastLoopStart (
		std::function<void(int contrast)> callback) override;
		
	stiHResult contrastLoopStop () override;

	void selfViewEnabledSet (
		bool enabled) override;

	bool RcuAvailableGet () const override;

	stiHResult ambientLightGet (
		IVideoInputVP2::AmbientLight *ambiance,
		float *rawVal) override;

	void videoRecordToFile() override;

	stiHResult EncodePipelineReset() override;
	stiHResult EncodePipelineDestroy() override;

	void currentTimeSet (
		const time_t currentTime) override;
		
	stiHResult VideoRecordSettingsSet (
		const SstiVideoRecordSettings *pVideoRecordSettings) override;

	void systemSleepSet(bool enabled) override;

	stiHResult VideoRecordStart () override;

protected:

	stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps) override;

	MonitorTaskBase *monitorTaskBaseGet () override;

	CstiGStreamerVideoGraphBase &videoGraphBaseGet () override;
	const CstiGStreamerVideoGraphBase &videoGraphBaseGet () const override;

	//
	// Member functions
	//
	void ipuFocusCompleteCallback (bool success, int contrast);

	stiHResult focusWindowGet (
		int *pnStartX,
		int *pnStartY,
		int *pnWinW,
		int *pnWinH);

	//
	// Event Handlers (run from event queue)
	//
	void eventFocusComplete (bool success, int contrast);
	void eventFocusPositionSet (int focusPosition);

private:
	void eventAutoFocusTimerTimeout ();
	void eventMarshalEncodeSettingsSet ();
	void eventEncodeSettingsSet ();
	
	stiHResult NoLockCameraOpen () override;
	stiHResult NoLockCameraClose () override;
	stiHResult NoLockRcuStatusChanged () override;
	stiHResult NoLockCameraSettingsSet () override;

	void NoLockWindowScale (
		int originalWidth,
		int originalHeight,
		CstiCameraControl::SstiPtzSettings *exposureWindow);

	stiHResult NoLockAutoExposureWindowSet () override;

	stiHResult SelfViewWidgetSet (
		void * QQuickItem) override;
	
	stiHResult PlaySet (
		bool play) override;

	void eventVideoRecordToFile();

	void pipelineEnabledSet (bool set);

protected:

	int m_focusPosition {0};

	// Quit after 5 seconds
	const int AUTO_FOCUS_WAIT_TIME_MS {5000};
	
	// Amount of time to wait after shuting down capture pipeline (icamerasrc limitation)
	// before allowing it be created again. It also limits the speed that we 
	// can shutdown after creating but that is not a problem.
	const int PIPELINE_SHUTDOWN_WAIT_TIME_MS {500};
	
	const int PIPELINE_SIZE_CHANGE_WAIT_TIME_MS {1000};

	CstiTimer m_autoFocusTimer {AUTO_FOCUS_WAIT_TIME_MS};

	CstiMonitorTask *m_monitorTask {nullptr};
	CstiGStreamerVideoGraph m_gstreamerVideoGraph;

private:
	std::chrono::steady_clock::time_point m_lastSelfViewEnableTime = {};
	
	std::chrono::steady_clock::time_point m_lastEncodeSettingsSetTime = {};
	
	vpe::EventTimer m_encodeSettingsSetTimer;
	vpe::EventTimer m_selfviewDisabledIntervalTimer;
	vpe::EventTimer m_selfviewDisabledStartTimer;
	vpe::EventTimer m_selfviewDisabledStopTimer;
	vpe::EventTimer m_selfviewTransitionTimer;
	
	bool m_encodeSettingsSetQueued {false};
	bool m_videoRecordToFileSet {false};
	bool m_videoEncodeStart {false};

	bool m_selfviewEnabled {true};

	bool m_allowAmbientLightGet {true};

	bool m_systemSleepEnabled {false};

	IVideoInputVP2::AmbientLight m_lastAmbientLight {IVideoInputVP2::AmbientLight::UNKNOWN};

	CstiSignalConnection::Collection m_videoOutputSignalConnections;
};
