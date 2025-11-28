#include "CropScaleBin.h"
#include "GStreamerElementFactory.h"



CropScaleBin::CropScaleBin (std::string name, bool enableScale, bool enableConvert)
:
	CropScaleBinBase(name, enableScale, enableConvert)
{
}

void CropScaleBin::create()
{
	auto hResult = stiRESULT_SUCCESS;
	bool result = false;

	GStreamerElement capturePTZ;
	GStreamerElement ptzScaleFilter;
	GStreamerElement captureConvert;
	GStreamerElement captureScale;
	GStreamerCaps gstCaps;
	GStreamerPad sinkPad;
	GStreamerPad srcPad;

	clear ();
	GStreamerBin::create (binName, false);

	capturePTZ = GStreamerElement("videocrop", PTZ_ELEMENT_NAME);
	stiTESTCOND(capturePTZ.get (), stiRESULT_ERROR);

	elementAdd (capturePTZ);

	if (m_enableConvert)
	{
		captureConvert = GStreamerElement("autovideoconvert", "capture_convert");
		stiTESTCOND(captureConvert.get(), stiRESULT_ERROR);

		elementAdd (captureConvert);

		result = captureConvert.link(capturePTZ);
		stiTESTCOND(result, stiRESULT_ERROR);
	
		sinkPad = captureConvert.staticPadGet ("sink");
	}
	else
	{
		sinkPad = capturePTZ.staticPadGet ("sink");
	}

	stiTESTCOND(sinkPad.get(), stiRESULT_ERROR);

	result = ghostPadAdd ("sink", sinkPad);
	stiTESTCOND(result, stiRESULT_ERROR);

	if (m_enableScale)
	{
		captureScale = GStreamerElement("videoscale", "capture_scale");
		stiTESTCOND(captureScale.get(), stiRESULT_ERROR);

		elementAdd (captureScale);

		result = capturePTZ.link(captureScale);
		stiTESTCOND(result, stiRESULT_ERROR);

		ptzScaleFilter = GStreamerElement ("capsfilter", PTZ_FILTER_ELEMENT_NAME);
		stiTESTCOND(ptzScaleFilter.get (), stiRESULT_ERROR);

		elementAdd (ptzScaleFilter);

		result = captureScale.link (ptzScaleFilter);
		stiTESTCOND(result, stiRESULT_ERROR);
	
		srcPad = ptzScaleFilter.staticPadGet("src");
	}
	else
	{
		srcPad = capturePTZ.staticPadGet("src");
	}

	stiTESTCOND(srcPad.get (), stiRESULT_ERROR);

	result = ghostPadAdd("src", srcPad);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		clear ();
	}
}


stiHResult CropScaleBin::cropRectangleSet (
	const CstiCameraControl::SstiPtzSettings &ptzRectangle, int frameWidthIn, int frameHeightIn)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	stiDEBUG_TOOL (g_stiVideoInputDebug,
		vpe::trace ("CropScaleBin::cropRectangleSet\n");
	);
	
	if (!empty())
	{
		GStreamerElement ptzFilter;
		std::stringstream ss;

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CropScaleBin::cropRectangleSet: setting crop on element", "\n");
			vpe::trace ("crop-top = ", ptzRectangle.vertPos, "\n");
			vpe::trace ("crop-bottom = ", frameHeightIn - ptzRectangle.vertZoom - ptzRectangle.vertPos, "\n");
			vpe::trace ("crop-left = ", frameWidthIn - ptzRectangle.horzZoom - ptzRectangle.horzPos, "\n");
			vpe::trace ("crop-right = ", ptzRectangle.horzPos, "\n");
		);
		
		auto ptzElement = elementGetByName (PTZ_ELEMENT_NAME);
		stiTESTCONDMSG (ptzElement.get () != nullptr, stiRESULT_ERROR, "CropScaleBin::cropRectangleSet: Error - No PTZ element\n");

		ptzElement.propertySet ("top", static_cast<guint64>(ptzRectangle.vertPos));
		ptzElement.propertySet ("bottom", static_cast<guint64>(frameHeightIn - ptzRectangle.vertZoom - ptzRectangle.vertPos));
		// crop-left and crop-right are 'reversed' due to mirroring
		ptzElement.propertySet ("left", static_cast<guint64>(frameWidthIn - ptzRectangle.horzZoom - ptzRectangle.horzPos));
		ptzElement.propertySet ("right", static_cast<guint64>(ptzRectangle.horzPos));

		if (m_enableScale && frameWidthIn > 0 && frameHeightIn > 0)
		{
			auto scaleFilterElement = elementGetByName (PTZ_FILTER_ELEMENT_NAME);
			stiTESTCONDMSG (scaleFilterElement.get () != nullptr, stiRESULT_ERROR, "CropScaleBin::cropRectangleSet: Error - No PTZ element\n");

			std::string capsStr = "video/x-raw,width=";
			capsStr += std::to_string(frameWidthIn);
			capsStr += ",height=";
			capsStr += std::to_string(frameHeightIn);

			GStreamerCaps gstCaps = GStreamerCaps(capsStr);
			stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);

			scaleFilterElement.propertySet("caps", gstCaps);
		}
	}
	
STI_BAIL:
	return hResult;
}
