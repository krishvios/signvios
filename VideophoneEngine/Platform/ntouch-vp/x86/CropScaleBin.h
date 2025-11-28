#pragma once

#include <gst/gst.h>
#include "stiDefs.h"
#include "stiSVX.h"
#include "GStreamerAppSink.h"
#include "CropScaleBinBase.h"


class CropScaleBin : public CropScaleBinBase
{
public:

	CropScaleBin (std::string name, bool enableScale = true, bool enableConvert = false);
	~CropScaleBin () override = default;

	CropScaleBin (const CropScaleBin &other) = delete;
	CropScaleBin (CropScaleBin &&other) = delete;
	CropScaleBin &operator= (const CropScaleBin &other) = delete;
	CropScaleBin &operator= (CropScaleBin &&other) = delete;

	void create();

	stiHResult cropRectangleSet (
		const CstiCameraControl::SstiPtzSettings &ptzRectangle, int frameWidthIn, int frameHeightIn);

protected:

private:
	
};
