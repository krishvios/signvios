#pragma once

#include "MediaPipeline.h"
#include "GStreamerAppSink.h"
#include "stiTrace.h"
#include "CstiCameraControl.h"
#include "CaptureBinBase.h"
#include "PropertyRange.h"
#include "IstiVideoOutput.h"
#include <functional>

class EncodePipelineBase: public MediaPipeline
{
public:

	EncodePipelineBase ();

	void displaySinkProbeAdd ();

	void frameCountGet (
		uint64_t *frameCount,
		std::chrono::steady_clock::time_point *lastFrameTime);

	void frameCaptureCallbackSet (
		std::function <void(const GStreamerPad &gstPad, GStreamerBuffer &gstBuffer)> callback);

	CstiSignal<GStreamerSample &> encodeBitstreamReceivedSignal;

	virtual void ptzRectangleSet (
		const CstiCameraControl::SstiPtzSettings &ptzRectangle);

	virtual float aspectRatioGet () const;

	CstiSignal<> aspectRatioChanged;

	virtual void encodeSizeSet (unsigned width, unsigned height) = 0;
	virtual void codecSet (
		EstiVideoCodec codec,
		EstiVideoProfile profile,
		unsigned int constraints,
		unsigned int level,
		EstiPacketizationScheme packetizationMode) = 0;
	virtual void bitrateSet (unsigned kbps) = 0;
	virtual void framerateSet (unsigned fps) = 0;

	virtual void encodingStart () = 0;
	virtual void encodingStop () = 0;

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
		IVideoInputVP2::AmbientLight &ambientLight,
		float &rawAmbientLight)
	{
		return stiRESULT_SUCCESS;
	}

	virtual void recordBinInitialize(bool recordFastStartFile) = 0;

	// Functions needed only for VP2
	bool isEncoding ()
	{
		return m_encoding;
	}

	virtual int maxCaptureWidthGet () const;
	virtual int maxCaptureHeightGet () const;

protected:

	GstFlowReturn sampleFromAppSinkPull (
		GStreamerAppSink appSink);

	virtual GStreamerElement displaySinkGet () = 0;

	virtual const CaptureBinBase &captureBinBaseGet () const = 0;
	virtual CaptureBinBase &captureBinBaseGet () = 0;

	bool m_encoding {false};

private:

	std::mutex m_frameCountMutex;
	uint64_t m_frameCount {0};
	std::chrono::steady_clock::time_point m_lastFrameTime {};

	std::function <void(const GStreamerPad &gstPad, GStreamerBuffer &gstBuffer)> m_frameCaptureCallback;

	void frameCountIncrement ();

	GStreamerSample m_currentGstSample;
};
