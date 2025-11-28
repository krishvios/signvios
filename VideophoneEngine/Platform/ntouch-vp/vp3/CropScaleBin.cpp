#include "CropScaleBin.h"
#include "GStreamerElementFactory.h"

CropScaleBin::CropScaleBin (std::string name, bool enableScale)
:
	CropScaleBinBase(name, enableScale)
{
}


void CropScaleBin::create()
{
	auto hResult = stiRESULT_SUCCESS;
	bool result = false;

	GStreamerElement capturePTZ;
	GStreamerElement ptzScaleFilter;
	GStreamerCaps gstCaps;
	GStreamerPad sinkPad;
	GStreamerPad srcPad;

	clear ();
	GStreamerBin::create (binName, false);

	GStreamerElementFactory vppFactory ("msdkvpp");
	stiTESTCOND(vppFactory.get (), stiRESULT_ERROR);

	capturePTZ = vppFactory.createElement(PTZ_ELEMENT_NAME);
	stiTESTCOND(capturePTZ.get (), stiRESULT_ERROR);
	
	// ScalingMode
	//MFX_SCALING_MODE_DEFAULT    = 0,
	//MFX_SCALING_MODE_LOWPOWER   = 1,
	//MFX_SCALING_MODE_QUALITY    = 2
	capturePTZ.propertySet ("scaling-mode", 2);

	ptzScaleFilter = GStreamerElement ("capsfilter", PTZ_FILTER_ELEMENT_NAME);
	stiTESTCOND(ptzScaleFilter.get (), stiRESULT_ERROR);

	gstCaps = GStreamerCaps ("video/x-raw(memory:DMABuf),format=NV12");
	stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);

	ptzScaleFilter.propertySet ("caps", gstCaps);

	elementAdd (capturePTZ);
	elementAdd (ptzScaleFilter);

	result = capturePTZ.link (ptzScaleFilter);
	stiTESTCOND(result, stiRESULT_ERROR);

	// Create a sink pad for the bin
	sinkPad = capturePTZ.staticPadGet ("sink");
	stiTESTCOND(sinkPad.get(), stiRESULT_ERROR);

	result = ghostPadAdd ("sink", sinkPad);
	stiTESTCOND(result, stiRESULT_ERROR);

	// Create a src pad for the bin
	srcPad = ptzScaleFilter.staticPadGet("src");
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
		GStreamerCaps gstCaps;
		GStreamerElement ptzFilter;
		std::stringstream ss;

		stiDEBUG_TOOL (g_stiVideoInputDebug,
			vpe::trace ("CaptureBin::cropRectangleSet: setting crop on element", "\n");
			vpe::trace ("crop-top = ", ptzRectangle.vertPos, "\n");
			vpe::trace ("crop-bottom = ", frameHeightIn - ptzRectangle.vertZoom - ptzRectangle.vertPos, "\n");
			vpe::trace ("crop-left = ", frameWidthIn - ptzRectangle.horzZoom - ptzRectangle.horzPos, "\n");
			vpe::trace ("crop-right = ", ptzRectangle.horzPos, "\n");
		);
		
		auto ptzElement = elementGetByName (PTZ_ELEMENT_NAME);
		stiTESTCONDMSG (ptzElement.get () != nullptr, stiRESULT_ERROR, "CropScaleBin::cropRectangleSet: Error - No PTZ element\n");

		ptzElement.propertySet ("crop-top", static_cast<guint64>(ptzRectangle.vertPos));
		ptzElement.propertySet ("crop-bottom", static_cast<guint64>(frameHeightIn - ptzRectangle.vertZoom - ptzRectangle.vertPos));
		// crop-left and crop-right are 'reversed' due to mirroring
		ptzElement.propertySet ("crop-left", static_cast<guint64>(frameWidthIn - ptzRectangle.horzZoom - ptzRectangle.horzPos));
		ptzElement.propertySet ("crop-right", static_cast<guint64>(ptzRectangle.horzPos));

		if (m_enableScale && frameWidthIn > 0 && frameHeightIn > 0)
		{
			auto scaleFilterElement = elementGetByName (PTZ_FILTER_ELEMENT_NAME);
			stiTESTCONDMSG (scaleFilterElement.get () != nullptr, stiRESULT_ERROR, "CropScaleBin::cropRectangleSet: Error - No PTZ element\n");

			std::string capsStr = "video/x-raw(memory:DMABuf),format=NV12,width=";
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
