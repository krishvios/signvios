#pragma once

#include "FrameCaptureBinBase.h"
#include "CaptureBin.h"
#include "CropScaleBin.h"

class FrameCaptureBin : public FrameCaptureBinBase
{
public:

	FrameCaptureBin ();
	~FrameCaptureBin () override = default;

	FrameCaptureBin (const FrameCaptureBin &other) = delete;
	FrameCaptureBin (FrameCaptureBin &&other) = delete;
	FrameCaptureBin &operator= (const FrameCaptureBin &other) = delete;
	FrameCaptureBin &operator= (FrameCaptureBin &&other) = delete;

	int maxCaptureWidthGet () const override
	{
		return stiMAX_CAPTURE_WIDTH;
	}

	int maxCaptureHeightGet () const override
	{
		return stiMAX_CAPTURE_HEIGHT;
	}

protected:

	CropScaleBinBase* cropScaleBinBaseGet () override
	{
		return &m_cropScaleBin;
	}

private:

	CropScaleBin m_cropScaleBin;
};
