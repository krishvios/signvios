#pragma once

#include "FrameCaptureBinBase.h"
#include "CaptureBin.h"
#include "CropScaleBin.h"

class FrameCaptureBin : public FrameCaptureBinBase
{
public:

	FrameCaptureBin (
		int maxCaptureWidth,
		int maxCaptureHeight)
	:
		m_maxCaptureWidth(maxCaptureWidth),
		m_maxCaptureHeight(maxCaptureHeight),
		m_cropScaleBin("framecap_ptz", true, true)
	{
	}

	~FrameCaptureBin () override = default;

	FrameCaptureBin (const FrameCaptureBin &other) = delete;
	FrameCaptureBin (FrameCaptureBin &&other) = delete;
	FrameCaptureBin &operator= (const FrameCaptureBin &other) = delete;
	FrameCaptureBin &operator= (FrameCaptureBin &&other) = delete;

	int maxCaptureWidthGet () const override
	{
		return m_maxCaptureWidth;
	}

	int maxCaptureHeightGet () const override
	{
		return m_maxCaptureHeight;
	}

protected:

	CropScaleBinBase* cropScaleBinBaseGet () override
	{
		return &m_cropScaleBin;
	}

private:

	int m_maxCaptureWidth;
	int m_maxCaptureHeight;

	CropScaleBin m_cropScaleBin;
};
