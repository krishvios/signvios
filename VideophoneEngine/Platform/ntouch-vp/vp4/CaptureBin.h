#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <linux/v4l2-mediabus.h>

#include "stiDefs.h"
#include "CaptureBinBase2.h"
#include "CstiCameraControl.h"
#include "IVideoInputVP2.h"
#include "CropScaleBin.h"
#include "sorensonVvcam.h"

class CaptureBin : public CaptureBinBase2
{
public:
	CaptureBin ();
	~CaptureBin ();

	CaptureBin (const CaptureBin &other) = delete;
	CaptureBin (CaptureBin &&other) = delete;
	CaptureBin &operator= (const CaptureBin &other) = delete;
	CaptureBin &operator= (CaptureBin &&other) = delete;

	stiHResult cropRectangleSet (
		const CstiCameraControl::SstiPtzSettings &ptzRectangle) override;

	stiHResult exposureWindowSet (const CstiCameraControl::SstiPtzSettings &exposureWindow) override
	{
		return stiRESULT_SUCCESS;
	}

	int maxCaptureWidthGet () const override;

	int maxCaptureHeightGet () const override;

	stiHResult brightnessGet (
		int *brightness) const override;

	stiHResult brightnessSet (
		int brightness) override;

	stiHResult saturationGet (
	 int *saturation) const override;

	stiHResult saturationSet (
		int saturation) override;

	stiHResult focusRangeGet (
		PropertyRange *range) const override;
		
	stiHResult focusPositionGet (
		int *focusPosition) const override;
	
	stiHResult focusPositionSet (
		int focusPosition) override;

	stiHResult singleFocus (
		int initialFocusPosition,
		int focusRegionStartX,
		int focusRegionStartY,
		int focusRegionWidth,
		int focusRegionHeight,
		std::function<void(bool success, int contrast)> callback) override;

protected:

	CropScaleBinBase &cropScaleBinBaseGet () override
	{
		return m_cropScaleBin;
	}

private:
	
	static constexpr const float AMBIENT_LIGHT_LIMIT_LOW {0.0439};
	static constexpr const float AMBIENT_LIGHT_LIMIT_MIDLOW {0.0550};
	static constexpr const float AMBIENT_LIGHT_LIMIT_MIDHIGH {0.1259};
	static constexpr const float AMBIENT_LIGHT_LIMIT_HIGH {0.1550};

	// From vvcam [-127 to 127]
	// Default can be customized
	static constexpr const int MIN_BRIGHTNESS_VALUE {-127};
	static constexpr const int MAX_BRIGHTNESS_VALUE {127};
	static constexpr const int DEFAULT_BRIGHTNESS_VALUE {0};
	static constexpr const int DEFAULT_BRIGHTNESS_STEP_SIZE {10};

	// From vvcam [0 to 1.99]
	// Default can be customized
	static constexpr const int MIN_SATURATION_VALUE {0};
	static constexpr const int MAX_SATURATION_VALUE {199};
	static constexpr const int DEFAULT_SATURATION_VALUE {100};
	static constexpr const int DEFAULT_SATURATION_STEP_SIZE {10};

	// TODO: Right now, we're using the DAC range from lumina1.
	// This needs to be updated after calibration is performed.
	static constexpr const int MIN_FOCUS_VALUE {1000};
	static constexpr const int MAX_FOCUS_VALUE {2800};
	static constexpr const int DEFAULT_FOCUS_VALUE {1600};
	static constexpr const int DAC_STEP_SIZE {10};
	
	void focusTask (
		int initialFocusPosition,
		int focusRegionStartX,
		int focusRegionStaryY,
		int focusRegionWidth,
		int focusRegionHeight,
		std::function<void(bool success, int contrast)> callback);
	bool m_focusInProcess {false};
	std::mutex m_cameraSrcMutex;

	void exposureWindowApply (
		GStreamerElement cameraElement,
		const CstiCameraControl::SstiPtzSettings &exposureWindow) override;

	GStreamerElement cameraElementGet () override;
	GStreamerCaps captureCapsGet () override;
	CropScaleBin m_cropScaleBin;

	sorensonVvcam m_vvcam;

};
