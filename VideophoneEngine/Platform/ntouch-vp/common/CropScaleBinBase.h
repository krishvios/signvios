#pragma once

#include "GStreamerBin.h"
#include "stiDefs.h"
#include "CstiCameraControl.h"

#define PTZ_ELEMENT_NAME "ptz_element"
#define PTZ_FILTER_ELEMENT_NAME "ptz_scale_filter_element"

class CropScaleBinBase : public GStreamerBin
{
public:
	CropScaleBinBase(std::string name, bool enableScale = true, bool enableConvert = false);
	~CropScaleBinBase () override = default;

	CropScaleBinBase (const CropScaleBinBase &other) = delete;
	CropScaleBinBase (CropScaleBinBase &&other) = delete;
	CropScaleBinBase &operator= (const CropScaleBinBase &other) = delete;
	CropScaleBinBase &operator= (CropScaleBinBase &&other) = delete;

	virtual stiHResult cropRectangleSet (const CstiCameraControl::SstiPtzSettings &ptzRectangle, 
				int frameWidthIn, int frameHeightIn) = 0;

	virtual void create () = 0;

protected:
	std::string binName;
	bool m_enableScale;
	bool m_enableConvert;

};
