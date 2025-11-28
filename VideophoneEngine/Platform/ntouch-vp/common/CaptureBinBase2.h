// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2021 Sorenson Communications, Inc. -- All rights reserved

#pragma once

#include "CstiCameraControl.h"
#include "CaptureBinBase.h"
#include "stiDefs.h"
#include "CropScaleBinBase.h"

// NOTE: Do not change this name without
// changing the string in MediaPipeline.cpp
#define CAMERA_ELEMENT_NAME "capture_camera_element"

class CaptureBinBase2 : public CaptureBinBase
{
public:
	CaptureBinBase2 (
		const PropertyRange &brightness,
		const PropertyRange &saturation);
	~CaptureBinBase2() override = default;

	CaptureBinBase2 (const CaptureBinBase2 &other) = delete;
	CaptureBinBase2 (CaptureBinBase2 &&other) = delete;
	CaptureBinBase2 &operator= (const CaptureBinBase2 &other) = delete;
	CaptureBinBase2 &operator= (CaptureBinBase2 &&other) = delete;

	void create ();

	virtual stiHResult exposureWindowSet (const CstiCameraControl::SstiPtzSettings &exposureWindow);

	virtual void exposureWindowApply (
		GStreamerElement cameraElement,
		const CstiCameraControl::SstiPtzSettings &exposureWindow) = 0;
protected:

	virtual CropScaleBinBase &cropScaleBinBaseGet () = 0;

private:

	CstiCameraControl::SstiPtzSettings m_exposureWindow {};

	virtual GStreamerElement cameraElementGet () = 0;
	virtual GStreamerCaps captureCapsGet () = 0;

};
