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
		const CstiCameraControl::SstiPtzSettings &ptzRectangle);
	
	stiHResult brightnessSet (
		int brightness) override;
	
	stiHResult saturationSet (
		int saturation) override;
	
	stiHResult exposureWindowSet (
		const CstiCameraControl::SstiPtzSettings &exposureWindow) override;

	int maxCaptureWidthGet () const override;

	int maxCaptureHeightGet () const override;

	stiHResult singleFocus (
		int initialFocusPosition,
		int focusRegionStartX,
		int focusRegionStartY,
		int focusRegionWidth,
		int focusRegionHeight,
		std::function<void(bool success, int contrast)> callback) override;
	
	stiHResult focusRangeGet (
		PropertyRange *range) const override;
		
	stiHResult focusPositionGet (
		int *focusPosition) const override;
	
	stiHResult focusPositionSet (
		int focusPosition) override;

	stiHResult contrastLoopStart (
		std::function<void(int contrast)> callback) override;
	stiHResult contrastLoopStop () override;
	
	stiHResult ambientLightGet (
		IVideoInputVP2::AmbientLight &ambientLight,
		float &rawAmbientLight) override;

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

	// Note:
	// During repeated testing of power cycling the camera and attempting to
	// obtain ambient light values prior to camera initialization, three default 
	// sensitivity values could be consistently observed, -Inf, 7.689464569091796875, 
	// and 24.570892333984375. These defaulted values result in innaccurate 
	// ambiance levels since the camera is not initialized. It is unknown why the camera 
	// defaults to these values at this time, values are not used but left for reference.
	//
	// static constexpr const float INVALID_SENSITIVITY_VALUE_1 {7.689464569091796875};
	// static constexpr const float INVALID_SENSITIVITY_VALUE_2 {24.570892333984375};
	
	// From v4l2 [-127 to 127]
	// Default can be customized
	static constexpr const int MIN_BRIGHTNESS_VALUE {-128};
	static constexpr const int MAX_BRIGHTNESS_VALUE {127};
	static constexpr const int DEFAULT_BRIGHTNESS_VALUE {0};
	static constexpr const int DEFAULT_BRIGHTNESS_STEP_SIZE {10};

	// From v4l2 [0 to 1.99]
	// Default can be customized
	static constexpr const int MIN_SATURATION_VALUE {-128};
	static constexpr const int MAX_SATURATION_VALUE {127};
	static constexpr const int DEFAULT_SATURATION_VALUE {0};
	static constexpr const int DEFAULT_SATURATION_STEP_SIZE {10};


	static constexpr const int MIN_FOCUS_VALUE {1000};
	static constexpr const int MAX_FOCUS_VALUE {2800};
	static constexpr const int DEFAULT_FOCUS_VALUE {1600};
	static constexpr const int DAC_STEP_SIZE {10};
	
	static constexpr const int FOCUS_WAIT_TIME_IN_MICROSECONDS = 33000;
	static constexpr const int MAX_FOCUS_TRIES = 120;

	void focusTask (
		int initialFocusPosition,
		int focusRegionStartX,
		int focusRegionStaryY,
		int focusRegionWidth,
		int focusRegionHeight,
		std::function<void(bool success, int contrast)> callback);
	bool m_focusInProcess {false};

	void contrastTask (
		std::function<void(int contrast)> callback);
	std::thread m_contrastThread;
	bool m_runContrastLoop {false};
	std::condition_variable m_contrastCondVar;
	std::mutex m_cameraSrcMutex;
	
	stiHResult focusVcmOpen ();
	
	void focusVcmClose ();
	
	void exposureWindowApply (
		GStreamerElement cameraElement,
		const CstiCameraControl::SstiPtzSettings &exposureWindow) override;

	GStreamerElement cameraElementGet () override;
	GStreamerCaps captureCapsGet () override;
	CropScaleBin m_cropScaleBin;


	int m_vcmFd {-1};
	
	IVideoInputVP2::AmbientLight m_lastAmbientLight {IVideoInputVP2::AmbientLight::HIGH};

};
