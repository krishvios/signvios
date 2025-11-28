#include "CstiGStreamerVideoGraph.h"
#include "stiTools.h"

#define CROPRECT_BUFFER_SIZE 64
#define OUTPUT_SIZE_BUFFER_SIZE 64
#define DEFAULT_ENCODE_SLICE_SIZE 1300
#define DEFAULT_ENCODE_BIT_RATE 10000000
#define DEFAULT_ENCODE_FRAME_RATE 30
#define DEFAULT_ENCODE_FRAME_RATE_NUM 30000
#define DEFAULT_ENCODE_FRAME_RATE_DEN 1001
#define DEFAULT_ENCODE_WIDTH 1920
#define DEFAULT_ENCODE_HEIGHT 1072

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
	settings.ePacketization = estiH264_SINGLE_NAL;

	return settings;
};



CstiGStreamerVideoGraph::CstiGStreamerVideoGraph ()
:
	CstiGStreamerVideoGraphBase2 (true)
{
	m_VideoRecordSettings = defaultVideoRecordSettings ();
}

IstiVideoInput::SstiVideoRecordSettings CstiGStreamerVideoGraph::defaultVideoRecordSettingsGet ()
{
	return defaultVideoRecordSettings ();
}


std::unique_ptr<FrameCaptureBinBase> CstiGStreamerVideoGraph::frameCaptureBinCreate ()
{
	auto bin = sci::make_unique<FrameCaptureBin>();

	bin->create ();

	return bin;
}


float CstiGStreamerVideoGraph::aspectRatioGet ()
{
	return 1920.0 / 1080.0;
}
