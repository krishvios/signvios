// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiSLICRingerBase.h"
#include <chrono>
#undef TRUE // The VoicePath API and glib have conflicts in defining TRUE
#include "vp_api.h"
#include "lt_api.h"


class CstiSLICRinger : public CstiSLICRingerBase
{
public:

	CstiSLICRinger ();
	~CstiSLICRinger () override;

	CstiSLICRinger (const CstiSLICRinger &other) = delete;
	CstiSLICRinger (CstiSLICRinger &&other) = delete;
	CstiSLICRinger &operator = (const CstiSLICRinger &other) = delete;
	CstiSLICRinger &operator = (CstiSLICRinger &&other) = delete;

	// Used to initialize the VoicePath API
	// Must call base class Startup within
	stiHResult startup () override;

private:

	void eventInitialize ();
	void eventRingerStart () override;
	void eventRingerStop () override;
	void eventDeviceDetect () override;

	void lastDetectTimeGet ();
	void lastDetectTimeSet ();

	void slicEventGet ();
	void initSetMasks ();

	void lineTestEventProcess (VpEventType *event);

private:

	VpDevCtxType m_deviceContext {};
	Vp886DeviceObjectType m_deviceObject {};
	VpLineCtxType m_lineContext {};
	Vp886LineObjectType m_lineObject {};

	LtTestAttributesType m_testAttributes {};
	LtRingerInputType m_testInput {};
	LtRingersCriteriaType m_testCriteria {};
	LtTestTempType m_tempTestData {};
	LtTestResultType m_testResults {};
	LtTestCtxType m_testContext {};
	bool m_testInProgress {false};

	bool m_deviceInitialized = false;

	vpe::EventTimer m_eventPollTimer;

	std::chrono::system_clock::time_point m_lastDetectTime {};
};
