// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

/*!
 * \file CstiMonitorTask.h
 * \brief See CstiMonitorTask.cpp
 */

#ifndef CMONITOR_TASK_H
#define CMONITOR_TASK_H

//
// Includes
//
#include "MonitorTaskBase.h"
#include "IstiVideoOutput.h"
#include "IstiPlatform.h"
#include "stiTools.h"
#include "CstiTimer.h"
#include <media/ov640_v4l2.h>
#include <cstdint>
#include <memory>


//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//
struct udev;
struct udev_monitor;

//
// Globals
//
class CstiMonitorTask : public MonitorTaskBase
{
public:

	CstiMonitorTask ();

	~CstiMonitorTask () override;

	virtual stiHResult Startup ();

	virtual stiHResult Shutdown ();

	stiHResult Initialize ();

	virtual stiHResult HdmiInStatusGet (
		int *pnHdmiInStatus);

	virtual stiHResult HdmiInResolutionGet (
		int *pnHdmiInResolution);

	stiHResult RcuConnectedStatusGet (
		bool *pbRcuConnected) override;

	virtual stiHResult RcuLensTypeGet (
		int *lensType);

	virtual stiHResult MfddrvdRunningGet (
		bool *pbMfddrvdRunning);

	virtual stiHResult HdmiOutCapabilitiesGet (
		bool *pbConnected,
		uint32_t *pun32HdmiOutCapabilitiesBitMask,
		float * pf1080pRate,
		float * pf720pRate);

	virtual stiHResult HdmiOutDisplayModeSet (
		EDisplayModes DisplayMode,
		float fRate);

	virtual stiHResult HdmiOutDisplayRateGet (
		EDisplayModes eDisplayMode,
		float * pfRate);

	stiHResult mpuSerialNotify (
		std::string mpuSerialNumber) override;

	bool HdmiHotPlugGet ();

	bool headphoneConnectedGet ();

	bool microphoneConnectedGet ();

public:
	//
	// Event Signals
	//
	CstiSignal<> mfddrvdStatusChangedSignal;
	CstiSignal<int> hdmiInStatusChangedSignal;
	CstiSignal<> hdmiHotPlugStateChangedSignal;
	CstiSignal<bool> headphoneConnectedSignal;
	CstiSignal<bool> microphoneConnectedSignal;

private:
	stiHResult HdmiInStatusChanged ();

	stiHResult HdmiInStatusUpdate (
		bool *pbStatusChanged);

	stiHResult HdmiInResolutionUpdate (
		bool *pbResolutionChanged);

	stiHResult HdmiOutCapabilitiesChanged ();

	stiHResult HdmiOutCapabilitiesUpdate (
		bool *pbCapabilitiesChanged);

	stiHResult RcuLensTypeSet ();

	void HdmiOutHotPlugCheck ();

	void AudioJackStatusInitialize();

private:
	void EventUdevReadHandler();
	void EventBounceTimerTimeout ();

private:
	bool m_bInitialized = false;
	bool m_bInBounce = false;

	int m_nHdmiInStatus = 0;
	int m_nHdmiInResolution = 0;

	CstiTimer m_bounceTimer;
	CstiSignalConnection m_bounceTimerConnection;

	bool m_bHdmiOutConnected = false;
	uint32_t m_un32HdmiOutCapabilitiesBitMask = 0;
	float m_fHdmiOut1080pRate = 0;
	float m_fHdmiOut720pRate = 0;

	int m_rcuLensType = OV640_LENS_TYPE_9513A_LARGAN;
	bool m_bRcuConnected = false;
	bool m_bMfddrvdRunning = false;

	bool m_bHdmiFixed = false;

	int m_udevFd = -1;
	struct udev *m_pUdev = nullptr;
	struct udev_monitor *m_pUdevMonitor = nullptr;

	bool m_bHotPlugDetected = false;

	bool m_headphone {false};
};


#endif //#ifndef CMONITORTASK_H
