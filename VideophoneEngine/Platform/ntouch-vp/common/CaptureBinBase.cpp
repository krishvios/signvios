#include "CaptureBinBase.h"
#include "stiDebugTools.h"
#include "stiError.h"
#include "stiTrace.h"

CaptureBinBase::CaptureBinBase (
	const PropertyRange &brightness,
	const PropertyRange &saturation)
:
	GStreamerBin ("capture_bin"),
	m_brightnessRange(brightness),
	m_saturationRange(saturation)
{
}

stiHResult CaptureBinBase::brightnessRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	*range = m_brightnessRange;

	return hResult;
};

stiHResult CaptureBinBase::brightnessGet (
	int *brightness) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CaptureBinBase::brightnessGet - m_brightness: %d\n", m_brightness);
	);

	*brightness = m_brightness;

	return hResult;
}

stiHResult CaptureBinBase::brightnessSet (
	int brightness)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	stiTESTCOND (brightness >= m_brightnessRange.min && brightness <= m_brightnessRange.max, stiRESULT_ERROR);

	m_brightness = brightness;

STI_BAIL:

	return hResult;
}

stiHResult CaptureBinBase::saturationRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	*range = m_saturationRange;

	return hResult;
};

stiHResult CaptureBinBase::saturationGet (
	int *saturation) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		stiTrace ("CaptureBinBase::saturationGet - m_saturation: %d\n", m_saturation);
	);

	*saturation = m_saturation;

	return hResult;
}

stiHResult CaptureBinBase::saturationSet (
	int saturation)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	stiTESTCOND (saturation >= m_saturationRange.min && saturation <= m_saturationRange.max, stiRESULT_ERROR);

	m_saturation = saturation;

STI_BAIL:

	return hResult;
}
