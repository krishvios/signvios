// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved

//
// Includes
//
#include "CstiGStreamerVideoGraph.h"
#include "GStreamerAppSink.h"
#include "GStreamerAppSource.h"
#include "stiTrace.h"
#include "IstiVideoInput.h"
#include "CstiEvent.h"
#include "CstiVideoInput.h"
#include "IstiPlatform.h"
#include "CstiBSPInterface.h"
#include "CaptureBin.h"
#include "EncodePipeline.h"

#include "CaptureBin.h"
#include "EncodePipeline.h"

#include <cmath>
#include <functional>

//
// Constant
//

//
// Typedefs
//


#define CROPRECT_BUFFER_SIZE 64
#define OUTPUT_SIZE_BUFFER_SIZE 64
#define DEFAULT_ENCODE_SLICE_SIZE 1300
#define DEFAULT_ENCODE_BIT_RATE 10000000
#define DEFAULT_ENCODE_FRAME_RATE 30
#define DEFAULT_ENCODE_FRAME_RATE_NUM 30000
#define DEFAULT_ENCODE_FRAME_RATE_DEN 1001
#define DEFAULT_ENCODE_WIDTH 1920
#define DEFAULT_ENCODE_HEIGHT 1080

static IstiVideoInput::SstiVideoRecordSettings defaultVideoRecordSettings ()
{
	IstiVideoInput::SstiVideoRecordSettings settings;

	settings.unMaxPacketSize = DEFAULT_ENCODE_SLICE_SIZE;
	settings.unTargetBitRate = DEFAULT_ENCODE_BIT_RATE;
	settings.unTargetFrameRate = DEFAULT_ENCODE_FRAME_RATE;
	settings.unRows = DEFAULT_ENCODE_HEIGHT;
	settings.unColumns = DEFAULT_ENCODE_WIDTH;
	settings.eProfile = estiH264_BASELINE;
	settings.unConstraints = 0xe0;
	settings.unLevel = estiH264_LEVEL_4_1;
	settings.ePacketization = estiH264_NON_INTERLEAVED;
	settings.recordFastStartFile = false;

	return settings;
};


CstiGStreamerVideoGraph::CstiGStreamerVideoGraph ()
:
	CstiGStreamerVideoGraphBase2(true)
{
	m_VideoRecordSettings = defaultVideoRecordSettings ();
}

#define DISPLAY_MULTIPLE 2

stiHResult CstiGStreamerVideoGraph::DisplaySettingsSet (
	IstiVideoOutput::SstiImageSettings *pImageSettings)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	// Fixup settings to be even
//	pImageSettings->unPosX = pImageSettings->unPosX - (pImageSettings->unPosX % DISPLAY_MULTIPLE);
//	pImageSettings->unPosY = pImageSettings->unPosY - (pImageSettings->unPosY % DISPLAY_MULTIPLE);
	pImageSettings->unWidth = pImageSettings->unWidth - (pImageSettings->unWidth % DISPLAY_MULTIPLE);
	pImageSettings->unHeight = pImageSettings->unHeight - (pImageSettings->unHeight % DISPLAY_MULTIPLE);

	m_DisplayImageSettings = *pImageSettings;

	stiDEBUG_TOOL (g_stiVideoInputDebug || g_stiVideoSizeDebug,
		stiTrace ("CstiGStreamerVideoGraph::DisplaySettingsSet: m_bEncoding = %d, m_DisplayImageSettings.bVisible = %d (%d, %d, %d, %d)\n",
			m_bEncoding,
			m_DisplayImageSettings.bVisible,
			m_DisplayImageSettings.unPosX, m_DisplayImageSettings.unPosY, m_DisplayImageSettings.unWidth, m_DisplayImageSettings.unHeight);
	);
	
	return (hResult);
}

IstiVideoInput::SstiVideoRecordSettings CstiGStreamerVideoGraph::defaultVideoRecordSettingsGet ()
{
	return defaultVideoRecordSettings ();
}


std::unique_ptr<FrameCaptureBinBase> CstiGStreamerVideoGraph::frameCaptureBinCreate ()
{
	auto pipeline = sci::make_unique<FrameCaptureBin>(
		maxCaptureWidthGet(),
		maxCaptureHeightGet());

	pipeline->create ();

	return pipeline;
}
