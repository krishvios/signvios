#pragma once

#include "CaptureBinBase2.h"
#include "CropScaleBin.h"


#define stiMAX_CAPTURE_WIDTH 3840
#define stiMAX_CAPTURE_HEIGHT 2160

class CaptureBin : public CaptureBinBase2
{
public:

	CaptureBin ();
	~CaptureBin () override = default;

	CaptureBin (const CaptureBin &other) = delete;
	CaptureBin (CaptureBin &&other) = delete;
	CaptureBin &operator= (const CaptureBin &other) = delete;
	CaptureBin &operator= (CaptureBin &&other) = delete;

	stiHResult brightnessSet (int brightness) override
	{
		return stiRESULT_SUCCESS;
	}
		 
	stiHResult saturationSet (int saturation) override 
	{
		return stiRESULT_SUCCESS;
	}

	stiHResult exposureWindowSet (const CstiCameraControl::SstiPtzSettings &exposureWindow) override
	{
		return stiRESULT_SUCCESS;
	}

	stiHResult cropRectangleSet (const CstiCameraControl::SstiPtzSettings &ptzRectangle); 

	int maxCaptureWidthGet () const override
	{
		return stiMAX_CAPTURE_WIDTH;
	}

	int maxCaptureHeightGet () const override
	{
		return stiMAX_CAPTURE_HEIGHT;
	}

protected:

	CropScaleBinBase &cropScaleBinBaseGet () override
	{
		return m_cropScaleBin;
	}

private:

	// Default can be customized
	static constexpr const int MIN_BRIGHTNESS_VALUE {0};
	static constexpr const int MAX_BRIGHTNESS_VALUE {19};
	static constexpr const int DEFAULT_BRIGHTNESS_VALUE {5};
	static constexpr const int DEFAULT_BRIGHTNESS_STEP_SIZE {10};

	// Default can be customized
	static constexpr const int MIN_SATURATION_VALUE {0};
	static constexpr const int MAX_SATURATION_VALUE {10};
	static constexpr const int DEFAULT_SATURATION_VALUE {5};
	static constexpr const int DEFAULT_SATURATION_STEP_SIZE {10};

	void exposureWindowApply(
		GStreamerElement cameraElement,
		const CstiCameraControl::SstiPtzSettings &exposureWindow) override;

	GStreamerElement cameraElementGet () override;
	GStreamerCaps captureCapsGet () override;

	CropScaleBin m_cropScaleBin;
};
