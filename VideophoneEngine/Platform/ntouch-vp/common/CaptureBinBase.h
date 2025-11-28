#pragma once

#include "CstiCameraControl.h"
#include "GStreamerBin.h"
#include "PropertyRange.h"
#include "IVideoInputVP2.h"

class CaptureBinBase: public GStreamerBin
{
public:

	CaptureBinBase (
		const PropertyRange &brightness,
		const PropertyRange &saturation);

	virtual stiHResult cropRectangleSet (const CstiCameraControl::SstiPtzSettings &ptzRectangle)
	{
		m_cropRectangle = ptzRectangle;
		
		stiDEBUG_TOOL (g_stiVideoInputDebug,
			stiTrace ("CaptureBinBase::cropRectangleSet: Zoom Window: X: %d, Y: %d, W: %d, H: %d\n", 3840 - 1 - (m_cropRectangle.horzPos + m_cropRectangle.horzZoom - 1), m_cropRectangle.vertPos, m_cropRectangle.horzZoom, m_cropRectangle.vertZoom);
		);

		return stiRESULT_SUCCESS;
	}

	virtual CstiCameraControl::SstiPtzSettings cropRectangleGet () const
	{
		return m_cropRectangle;
	}

	virtual int maxCaptureWidthGet () const = 0;
	virtual int maxCaptureHeightGet () const = 0;

	virtual stiHResult brightnessRangeGet (
		PropertyRange *range) const;

	virtual stiHResult brightnessGet (
		int *brightness) const;

	virtual stiHResult brightnessSet (
		int brightness);

	virtual stiHResult saturationRangeGet (
		PropertyRange *range) const;

	virtual stiHResult saturationGet (
		int *saturation) const;

	virtual stiHResult saturationSet (
		int saturation);
	
	virtual stiHResult focusRangeGet (
		PropertyRange *range) const
	{
		return stiRESULT_SUCCESS;
	}
		
	virtual stiHResult focusPositionGet (
		int *focusPosition) const
	{
		return stiRESULT_SUCCESS;
	}
	
	virtual stiHResult focusPositionSet (
		int focusPosition)
	{
		return stiRESULT_SUCCESS;
	}

	virtual stiHResult singleFocus (
		int initialFocusPosition,
		int focusRegionStartX,
		int focusRegionStartY,
		int focusRegionWidth,
		int focusRegionHeight,
		std::function<void(bool success, int contrast)> callback)
	{
		return stiRESULT_SUCCESS;
	}

	virtual stiHResult contrastLoopStart (
		std::function<void(int contrast)> callback)
	{
		return stiRESULT_SUCCESS;
	}

	virtual stiHResult contrastLoopStop ()
	{
		return stiRESULT_SUCCESS;
	}
	
	virtual stiHResult ambientLightGet (
		IVideoInputVP2::AmbientLight &ambiance,
		float &rawVal)
	{
		return stiRESULT_SUCCESS;
	}

protected:

private:

	const PropertyRange m_brightnessRange;
	const PropertyRange m_saturationRange;

	int m_brightness {0};
	int m_saturation {0};

	CstiCameraControl::SstiPtzSettings m_cropRectangle {};
};
