#include "EncodePipelineBase2.h"
#include "stiSVX.h"
#include "stiTools.h"

void EncodePipelineBase2::create(void *qquickItem)
{
	stiHResult hResult {stiRESULT_ERROR};
	bool result = false;

	clear ();
	GStreamerPipeline::create("encode_capture_pipeline");

	captureBinCreate ();
	stiTESTCOND (captureBinBase2Get().get (), stiRESULT_ERROR);

	elementAdd(captureBinBase2Get());

	m_selfViewBin = nullptr;

	if (qquickItem)
	{
		m_selfViewBin = selfViewBinCreate(qquickItem);
		stiTESTCOND (m_selfViewBin.get() , stiRESULT_ERROR);
		stiTESTCOND (m_selfViewBin->get (), stiRESULT_ERROR);

		if (m_selfViewBin)
		{
			elementAdd(*m_selfViewBin);

			result = captureBinBase2Get().linkPads ("selfviewsrc", *m_selfViewBin, "sink");
			stiTESTCOND (result, stiRESULT_ERROR);
		}
	}

	if (!g_stiVideoExcludeEncode)
	{
		// This is so encode bin can have access to capture bin for looking at the cropping values
		m_encodeBin = encodeBinCreate(captureBinBase2Get());
		stiTESTCOND (m_encodeBin.get(), stiRESULT_ERROR);
		stiTESTCOND (m_encodeBin->get (), stiRESULT_ERROR);

		m_encodeBin->newBufferCallbackSet(
			[this](GStreamerAppSink appSink)
			{
				sampleFromAppSinkPull(appSink);
			});

		elementAdd(*m_encodeBin);

		result = captureBinBase2Get().linkPads ("encodesrc", *m_encodeBin, "sink");
		stiTESTCOND (result, stiRESULT_ERROR);
	}

	busWatchEnable ();

	displaySinkProbeAdd ();

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		clear ();
	}
}

void EncodePipelineBase2::codecSet (
	EstiVideoCodec codec,
	EstiVideoProfile profile,
	unsigned int constraints,
	unsigned int level,
	EstiPacketizationScheme packetizationMode)
{
	if (m_encodeBin)
	{
		m_encodeBin->codecSet (codec, profile, constraints, level, packetizationMode);
	}
}


void EncodePipelineBase2::bitrateSet (
	unsigned kbps)
{
	if (m_encodeBin)
	{
		m_encodeBin->bitrateSet (kbps);
	}
}


void EncodePipelineBase2::framerateSet (
	unsigned fps)
{
	if (m_encodeBin)
	{
		m_encodeBin->framerateSet (fps);
	}
}


GStreamerElement EncodePipelineBase2::displaySinkGet ()
{
	if (m_selfViewBin)
	{
		return *m_selfViewBin;
	}

	return {};
}


void EncodePipelineBase2::encodeSizeSet (
	unsigned width,
	unsigned height)
{
	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 1,
		vpe::trace ("EncodePipelineBase2::encodeSizeSet: encode width = ", width, ", encode height = ", height, "\n");
	);
	
	if (m_encodeBin)
	{
		m_encodeBin->sizeSet (width, height);
	}
}


void EncodePipelineBase2::encodingStart ()
{
	if (m_encodeBin)
	{
		m_encodeBin->fileRecordSet(m_recordFile);
		m_encodeBin->encodingStart ();
	}
}


void EncodePipelineBase2::encodingStop ()
{
	if (m_encodeBin)
	{
		m_recordFile = false;
		m_encodeBin->encodingStop ();
	}
}


stiHResult EncodePipelineBase2::exposureWindowSet (
	const CstiCameraControl::SstiPtzSettings &exposureWindow)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = captureBinBase2Get().exposureWindowSet (exposureWindow);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}

void EncodePipelineBase2::recordBinInitialize(
	bool recordFastStartFile)
{
	if (m_encodeBin)
	{
		m_encodeBin->recordBinInitialize (recordFastStartFile);
		m_recordFile = true;
	}

}

void EncodePipelineBase2::ptzRectangleSet (
	const CstiCameraControl::SstiPtzSettings &ptzRectangle)
{
	captureBinBaseGet ().cropRectangleSet (ptzRectangle);
	
	// TODO: No up/down scale
	// Add this if we want to adjust the pipeline size everytime
	// the ptz rectangle changes
	//if (m_encodeBin)
	//{
	//	m_encodeBin->encoderFilterUpdate ();
	//}
}

stiHResult EncodePipelineBase2::brightnessRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().brightnessRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::brightnessGet (
	int *brightness) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().brightnessGet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::brightnessSet (
	int brightness)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().brightnessSet (brightness);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::saturationRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().saturationRangeGet (range);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::saturationGet (
	int *saturation) const
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().saturationGet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::saturationSet (
	int saturation)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().saturationSet (saturation);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::focusRangeGet (
	PropertyRange *range) const
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = captureBinBaseGet ().focusRangeGet (range);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}

stiHResult EncodePipelineBase2::focusPositionGet (
	int *focusPosition) const
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = captureBinBaseGet ().focusPositionGet (focusPosition);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}

stiHResult EncodePipelineBase2::focusPositionSet (
	int focusPosition)
{
	stiHResult hResult {stiRESULT_SUCCESS};
		
	hResult = captureBinBaseGet ().focusPositionSet (focusPosition);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}

stiHResult EncodePipelineBase2::singleFocus (
	int initialFocusPosition,
	int focusRegionStartX,
	int focusRegionStartY,
	int focusRegionWidth,
	int focusRegionHeight,
	std::function<void(bool success, int contrast)> callback)
{
	stiHResult hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().singleFocus (
		initialFocusPosition,
		focusRegionStartX,
		focusRegionStartY,
		focusRegionWidth,
		focusRegionHeight,
		callback);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::contrastLoopStart (
	std::function<void(int contrast)> callback)
{
	auto hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().contrastLoopStart (callback);
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::contrastLoopStop ()
{
	auto hResult {stiRESULT_SUCCESS};

	hResult = captureBinBaseGet ().contrastLoopStop ();
	stiTESTRESULT ();

STI_BAIL:

	return hResult;
}

stiHResult EncodePipelineBase2::ambientLightGet (
	IVideoInputVP2::AmbientLight &ambientLight,
	float &rawAmbientLight)
{
	stiHResult hResult {stiRESULT_SUCCESS};
	
	hResult = captureBinBaseGet ().ambientLightGet (ambientLight, rawAmbientLight);
	stiTESTRESULT ();
	
STI_BAIL:
	
	return hResult;
}

