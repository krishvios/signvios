#pragma once

#include "FrameCaptureBinBase.h"
#include "CaptureBin.h"
#include <gst/gst.h>

class FrameCaptureBin : public FrameCaptureBinBase
{
public:

	FrameCaptureBin () = default;
	~FrameCaptureBin () override = default;

	FrameCaptureBin (const FrameCaptureBin &other) = delete;
	FrameCaptureBin (FrameCaptureBin &&other) = delete;
	FrameCaptureBin &operator= (const FrameCaptureBin &other) = delete;
	FrameCaptureBin &operator= (FrameCaptureBin &&other) = delete;

	void create();

	virtual void frameCapture (
		const GStreamerPad &gstPad,
		GStreamerBuffer &gstBuffer,
		bool crop,
		SstiImageCaptureSize imageSize);

	void maxCaptureWidthSet(const int maxCaptureWidth)
	{
		m_maxCaptureWidth = maxCaptureWidth;
	}

	void frameCaptureJpegEncDropSet(
		bool frameCapture);

	int maxCaptureWidthGet () const override
	{
		return m_maxCaptureWidth < stiMAX_CAPTURE_WIDTH ? m_maxCaptureWidth : stiMAX_CAPTURE_WIDTH;
	}

	int maxCaptureHeightGet () const override
	{
		return stiMAX_CAPTURE_HEIGHT;
	}

protected:

	CropScaleBinBase* cropScaleBinBaseGet () override
	{
		stiHResult hResult = stiRESULT_SUCCESS; stiUNUSED_ARG(hResult);
		stiTHROWMSG(stiRESULT_ERROR, "FrameCaptureBin::cropScaleBinBaseGet: Warning, cropScaleBin not implemented for VP2\n");

STI_BAIL:
		return nullptr;
	}

private:

	int m_maxCaptureWidth {0};
	
	static gboolean DebugProbeFunction (
	 	GstPad * pGstPad,
		GstMiniObject * pGstMiniObject,
		gpointer pUserData);

	static gboolean eventFunction (
	 	GstPad * pGstPad,
		GstMiniObject * pGstMiniObject,
		gpointer pUserData);

	GStreamerElement m_gstElementFrameCaptureConvert;
};
