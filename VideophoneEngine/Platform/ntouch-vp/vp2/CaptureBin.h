// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CaptureBinBase.h"
#include "stiDefs.h"

#define stiMAX_CAPTURE_WIDTH 1920
#define stiMAX_CAPTURE_HEIGHT 1072 //because its the value under 1080 thats a multiple of 16

class CaptureBin : public CaptureBinBase
{
public:
	CaptureBin ();
	~CaptureBin() override;

	void create ();

	void rcuConnectedSet(bool rcuConnected);
	bool rcuConnectedGet() const;

	stiHResult cropRectangleSet (const CstiCameraControl::SstiPtzSettings &ptzRectangle) override;

	stiHResult brightnessSet (int brightness) override;

	stiHResult saturationSet (int saturation) override;

	int maxCaptureWidthGet () const override
	{
		return stiMAX_CAPTURE_WIDTH;
	}

	int maxCaptureHeightGet () const override
	{
		return stiMAX_CAPTURE_HEIGHT;
	}

	void cameraControlFdSet(int cameraControl);

private:

	// Default can be customized
	static constexpr const int MIN_BRIGHTNESS_VALUE {0x02};
	static constexpr const int MAX_BRIGHTNESS_VALUE {0x64};
	static constexpr const int DEFAULT_BRIGHTNESS_VALUE {0x22};
	static constexpr const int DEFAULT_BRIGHTNESS_STEP_SIZE {10};

	// Default can be customized
	static constexpr const int MIN_SATURATION_VALUE {0x10};
	static constexpr const int MAX_SATURATION_VALUE {0x60};
	static constexpr const int DEFAULT_SATURATION_VALUE {0x40};
	static constexpr const int DEFAULT_SATURATION_STEP_SIZE {10};

	stiHResult CameraCaptureCreate ();
	stiHResult PTZBinCreate (bool cameraConnected);
	stiHResult PTZBinAddAndLink ();
	stiHResult TeeWithQueuesCreate ();
	stiHResult TeeWithQueuesAddAndLink ();
	stiHResult PTZBinRemove ();

	GStreamerElement m_captureSource;
	GstElement *m_pGstElementPTZBin {nullptr};
	GstElement *m_pGstElementPTZPreVideoConvertFilter {nullptr};
	GstElement *m_pGstElementPTZVideoConvert {nullptr};
	GstElement *m_pGstElementEncodeTee {nullptr};
	GstElement *m_pGstElementEncodeQueue0 {nullptr};
	GstElement *m_pGstElementEncodeQueue1 {nullptr};
	GstPad *m_pGstPadTeeSrc0 {nullptr};
	GstPad *m_pGstPadTeeSrc1 {nullptr};

	bool m_rcuConnected {true};
	int m_cameraControlFd {-1};
};
