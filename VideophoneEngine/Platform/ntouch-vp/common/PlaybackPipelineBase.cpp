#include "PlaybackPipelineBase.h"

#include "GStreamerPad.h"
#include "stiError.h"
#include "stiTools.h"
#include "CaptureBinBase.h"
#include "CstiVideoOutputBase.h"
#include <boost/filesystem.hpp>
#include <cmath>
#include <array>
#include <sys/stat.h>
#include <sys/types.h>
#include <array>
		 
#define PLAYBACK_TEMPLATE_BUFFER_NAME "gstTempFile-XXXXXX"
#define DEFAULT_PLAYBACK_DIR "VideoPlayback/"
#define SOURCE_ELEMENT_NAME "https_src"

#define  MAX_RING_BUFFER_TIMER_COUNT  100  //  Allows ring buffer timeout for 5 seconds with no activity.
#define  CERT_FILE_MAX_COUNT 2

const std::array<std::string, CERT_FILE_MAX_COUNT> certList 
{
	"/usr/share/lumina/certs/gdig2.crt.pem",
	"/usr/share/lumina/certs/TrustedRoot-DigiCertCA.pem"
};

const uint32_t MAX_RING_BUFFER_SIZE = 10485760;  // 10 MB ;
//const uint32_t MAX_RING_BUFFER_SIZE =  5242880;  // 5 MB ;
//const uint32_t MAX_RING_BUFFER_SIZE =  2621440;  // 2.5 MB ;


PlaybackPipelineBase::PlaybackPipelineBase(const std::string &name)
:
	CstiEventQueue (name),
	m_ringBufferingTimer (RING_BUFFERING_TIMEOUT, this),
	m_updateTimer (UPDATE_PLAYBACK_TIMEOUT, this)
{
	m_signalConnections.push_back (m_ringBufferingTimer.timeoutSignal.Connect (
		[this] () {
			eventRingBufferingTimerTimeout ();
		}));
	m_signalConnections.push_back (m_updateTimer.timeoutSignal.Connect (
		[this] () {
			eventUpdatePlaybackTimerTimeout ();
		}));
}


PlaybackPipelineBase::~PlaybackPipelineBase()
{
	stateSet(GST_STATE_NULL);

	CstiEventQueue::StopEventLoop();

	m_ringBufferingTimer.stop ();
	m_updateTimer.stop ();
}

void PlaybackPipelineBase::create()
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;
	std::string location;

	// Make the temporary playback directory is empty.
	stiOSDynamicDataFolderGet(&m_playbackFilePath);
	m_playbackFilePath += DEFAULT_PLAYBACK_DIR;

	boost::filesystem::remove_all(m_playbackFilePath);
	mkdir(m_playbackFilePath.c_str(), 0777);

	result = CstiEventQueue::StartEventLoop();
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		clear ();
	}
}

stiHResult PlaybackPipelineBase::videoFileLoad(
	const std::string &server,
	const std::string &fileGUID,
	const std::string &clientToken,
	int maxDownloadSpeed,
	bool videoLoop)
{
	auto hResult = stiRESULT_SUCCESS;
	GstStateChangeReturn StateChangeReturn;
	
	// This is called from the CstiVideoOutput thread so we need to lock it.
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);
	
	if (m_qquickItem)
	{
		m_msdkFactory = GStreamerElementFactory ("msdkvpp");
		m_imxFactory = GStreamerElementFactory ("imxvideoconvert_g2d");
			
		hResult = playbackPipelineCreate(server, 
										 fileGUID,
										 clientToken,
										 maxDownloadSpeed);
		stiTESTRESULT();

		StateChangeReturn = stateSet(GST_STATE_PAUSED);
		stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

		m_fileLoop = videoLoop;

		// If we are downloading a video then start a timer to check the download progress.
		if (!server.empty())
		{
			m_ringBufferingTimer.start();
		}
		else
		{
			m_videoFileIsLocal = true;
		}
	}
	else
	{
		m_videoFileLoadStarting = true;
	}

	m_server = server;
	m_fileGUID = fileGUID;
	m_clientToken = clientToken;
	m_maxDownloadSpeed = maxDownloadSpeed;
	m_fileLoop = videoLoop;

	m_screenSaverPathName.clear();

STI_BAIL:

	return hResult;
}


stiHResult PlaybackPipelineBase::screensaverLoad(
	const std::string &filePathName,
	int cropRight,
	int cropBottom)
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;
	GstStateChangeReturn StateChangeReturn;
	std::string emptyStr;
	GStreamerElement videoConvertCrop;
	GStreamerPad sinkPad;
	GStreamerElement videoCropQueue;
	GStreamerElement imxConvertCrop;
	GStreamerPad srcPad;
	GStreamerCaps gstCaps;

	// This is called from the CstiVideoOutput thread so we need to lock it.
	std::lock_guard<std::recursive_mutex> lock(m_execMutex);

	if (m_qquickItem)
	{
		m_playbackVideoCropBin = GStreamerBin("playback_video_crop_bin");
		stiTESTCOND(m_playbackVideoCropBin.get (), stiRESULT_ERROR);

		videoConvertCrop = GStreamerElement("videoconvert", "video_convert_crop");
		stiTESTCOND(videoConvertCrop.get (), stiRESULT_ERROR);

		videoCropQueue = GStreamerElement("queue", "video_crop+queue");
		stiTESTCOND(videoCropQueue.get(), stiRESULT_ERROR);

		m_msdkFactory = GStreamerElementFactory ("msdkvpp");
		m_imxFactory = GStreamerElementFactory ("imxvideoconvert_g2d");
		if (m_msdkFactory.get())
		{
			m_videoCrop = m_msdkFactory.createElement("msdk_vpp");
			stiTESTCOND(m_videoCrop.get(), stiRESULT_ERROR);

			m_videoCrop.propertySet("crop-left", 0);
			m_videoCrop.propertySet("crop-top", 0);
			m_videoCrop.propertySet("crop-right", cropRight);
			m_videoCrop.propertySet("crop-bottom", cropBottom);

			m_videoCropCaps = GStreamerElement("capsfilter", "msdk_vpp_caps");
			stiTESTCOND(m_videoCropCaps.get(), stiRESULT_ERROR);

			gstCaps = GStreamerCaps ("video/x-raw(memory:DMABuf),format=NV12");
			stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);
			m_videoCropCaps.propertySet ("caps", gstCaps);

			m_playbackVideoCropBin.elementAdd(videoConvertCrop);
			m_playbackVideoCropBin.elementAdd(m_videoCrop);
			m_playbackVideoCropBin.elementAdd(m_videoCropCaps);
			m_playbackVideoCropBin.elementAdd(videoCropQueue);

			sinkPad = videoConvertCrop.staticPadGet("sink");
			stiTESTCOND(sinkPad.get (), stiRESULT_ERROR);

			result = m_playbackVideoCropBin.ghostPadAdd("sink", sinkPad);
			stiTESTCOND(result, stiRESULT_ERROR);

			videoConvertCrop.link(m_videoCrop);
			stiTESTCOND(result, stiRESULT_ERROR);

			m_videoCrop.link(m_videoCropCaps);
			stiTESTCOND(result, stiRESULT_ERROR);

			m_videoCropCaps.link(videoCropQueue);
			stiTESTCOND(result, stiRESULT_ERROR);

			srcPad = videoCropQueue.staticPadGet("src");
			stiTESTCOND(srcPad.get (), stiRESULT_ERROR);

			result = m_playbackVideoCropBin.ghostPadAdd("src", srcPad);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
		else if (m_imxFactory.get())
		{
			//Note: this element actually performs the crop using offsets configured on videocrop element
			imxConvertCrop = m_imxFactory.createElement("imx-convert-crop");
			stiTESTCOND(imxConvertCrop.get (), stiRESULT_ERROR);

			imxConvertCrop.propertySet ("videocrop-meta-enable", true);

			m_videoCrop = GStreamerElement("videocrop", "video_crop");
			stiTESTCOND(m_videoCrop.get(), stiRESULT_ERROR);	

			m_videoCrop.propertySet("left", 0);
			m_videoCrop.propertySet("top", 0);
			m_videoCrop.propertySet("right", cropRight);
			m_videoCrop.propertySet("bottom", cropBottom);

			m_videoCropCaps = GStreamerElement("capsfilter", "imx_crop_caps");
			stiTESTCOND(m_videoCropCaps.get(), stiRESULT_ERROR);

			gstCaps = GStreamerCaps ("video/x-raw,format=RGBA");
			stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);
			m_videoCropCaps.propertySet ("caps", gstCaps);

			m_playbackVideoCropBin.elementAdd(videoConvertCrop);
			m_playbackVideoCropBin.elementAdd(imxConvertCrop);
			m_playbackVideoCropBin.elementAdd(m_videoCrop);
			m_playbackVideoCropBin.elementAdd(m_videoCropCaps);
			m_playbackVideoCropBin.elementAdd(videoCropQueue);

			sinkPad = videoConvertCrop.staticPadGet("sink");
			stiTESTCOND(sinkPad.get (), stiRESULT_ERROR);

			result = m_playbackVideoCropBin.ghostPadAdd("sink", sinkPad);
			stiTESTCOND(result, stiRESULT_ERROR);

			videoConvertCrop.link(imxConvertCrop);
			stiTESTCOND(result, stiRESULT_ERROR);

			imxConvertCrop.link(m_videoCrop);
			stiTESTCOND(result, stiRESULT_ERROR);

			m_videoCrop.link(m_videoCropCaps);
			stiTESTCOND(result, stiRESULT_ERROR);

			m_videoCropCaps.link(videoCropQueue);
			stiTESTCOND(result, stiRESULT_ERROR);

			srcPad = videoCropQueue.staticPadGet("src");
			stiTESTCOND(srcPad.get (), stiRESULT_ERROR);

			result = m_playbackVideoCropBin.ghostPadAdd("src", srcPad);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
		else
		{
			m_videoCrop = GStreamerElement("videocrop", "video_crop");
			stiTESTCOND(m_videoCrop.get(), stiRESULT_ERROR);

			m_videoCrop.propertySet("left",0);
			m_videoCrop.propertySet("top", 0);
			m_videoCrop.propertySet("right", cropRight);
			m_videoCrop.propertySet("bottom", cropBottom);

			m_playbackVideoCropBin.elementAdd(videoConvertCrop);
			m_playbackVideoCropBin.elementAdd(m_videoCrop);
			m_playbackVideoCropBin.elementAdd(videoCropQueue);

			sinkPad = videoConvertCrop.staticPadGet("sink");
			stiTESTCOND(sinkPad.get (), stiRESULT_ERROR);

			result = m_playbackVideoCropBin.ghostPadAdd("sink", sinkPad);
			stiTESTCOND(result, stiRESULT_ERROR);

			videoConvertCrop.link(m_videoCrop);
			stiTESTCOND(result, stiRESULT_ERROR);

			m_videoCrop.link(videoCropQueue);
			stiTESTCOND(result, stiRESULT_ERROR);

			srcPad = videoCropQueue.staticPadGet("src");
			stiTESTCOND(srcPad.get (), stiRESULT_ERROR);

			result = m_playbackVideoCropBin.ghostPadAdd("src", srcPad);
			stiTESTCOND(result, stiRESULT_ERROR);
		}

#if 0
		sinkPad = m_videoCrop.staticPadGet("sink");
		gst_pad_add_probe (sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)DebugProbeFunction, this, nullptr);

		srcPad = m_videoCrop.staticPadGet("src");
		gst_pad_add_probe (srcPad.get(), GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)DebugProbeFunction, this, nullptr);
#endif

		// Create the playback pipeline.
		hResult = playbackPipelineCreate(emptyStr,
										 filePathName,
										 emptyStr,
										 0); 
		stiTESTRESULT();

		m_videoFileIsLocal = true;

		m_fileLoop = true;

		StateChangeReturn = stateSet(GST_STATE_PLAYING);
		stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);
	}
	else
	{
		m_videoFileLoadStarting = true;
	}

	m_screenSaverPathName = filePathName;
	m_cropRight = cropRight;
	m_cropBottom = cropBottom;

STI_BAIL:

	return hResult;
}


stiHResult PlaybackPipelineBase::playbackPipelineCreate (
	std::string server,
	std::string fileGUID,
	std::string clientToken,
	int maxDownloadSpeed)
{
    stiDEBUG_TOOL(g_stiVideoOutputDebug,
		vpe::trace(__PRETTY_FUNCTION__, " Starting server: ", server.c_str(), ", fileGUID: ", fileGUID.c_str(), ", clientToken: ", clientToken.c_str(), ", maxDownloadSpeed: ", maxDownloadSpeed, " \n");
	);
	
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;

	auto pipelineType = estiPIPELINE_TYPE_UNKNOWN;
	std::size_t foundH264 = fileGUID.find(".h264");
	std::size_t foundMp4 = fileGUID.find(".mp4");
	std::size_t foundH265 = fileGUID.find(".h265");
	std::size_t foundMp5 = fileGUID.find(".mp5");
	std::size_t foundH265InName = fileGUID.find("H265");

	GStreamerPad sinkPad;
	GStreamerElement deinterlace;
	GStreamerElement videoConvert1;
	GStreamerElement fakeSink;
	GStreamerElement upload;

	GStreamerElement videoConvert0;

	std::string pipelineCommand;

	// Create the playback pipeline;
	std::string pipelineName("playback_pipeline");
	GStreamerPipeline::create(pipelineName);
	stiTESTCOND(get (), stiRESULT_ERROR);

	// Create as much of the decode bin as possible.  We can't complete it until
	// typefind tells us what kind of stream we have.
	m_playbackDecodeBin = GStreamerBin("playback_decode_bin");
	stiTESTCOND(m_playbackDecodeBin.get (), stiRESULT_ERROR);

	elementAdd(m_playbackDecodeBin);

	// Check for h264 video stream.
	if ((foundH264 != std::string::npos ||
		foundMp4 != std::string::npos) &&
		foundH265InName == std::string::npos)
	{
		pipelineType = estiPIPELINE_TYPE_MP4;
	}
	else if (foundH265 != std::string::npos ||
			   foundMp5 != std::string::npos ||
			   foundH265InName != std::string::npos)
	{
		pipelineType = estiPIPELINE_TYPE_MP5;
	}

	if (pipelineType == estiPIPELINE_TYPE_UNKNOWN)
	{
		// Create the typefind used to determine the video stream type.
		m_typeFind = GStreamerElement("typefind", "type_find");
		stiTESTCOND(m_typeFind.get (), stiRESULT_ERROR);

		// Add the typefind callback so we can complete the pipeline when we know the file type.
		g_signal_connect(m_typeFind.get(), "have-type", (GCallback) streamTypeFound, this);

		m_playbackDecodeBin.elementAdd(m_typeFind);
	}

	createDisplayBin();

	if (server.empty())
	{
		// Add file src
		m_srcElement = GStreamerElement("filesrc", "file_src");
		stiTESTCOND(m_srcElement.get (), stiRESULT_ERROR);

		pipelineCommand = "/" + fileGUID;
		m_srcElement.propertySet("location", pipelineCommand);
		m_playbackDecodeBin.elementAdd(m_srcElement);

		if (m_typeFind.get())
		{
			result = m_srcElement.link(m_typeFind);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
		else
		{
			decodeElementsAdd(&m_srcElement,
							  pipelineType);
		}
	}
	else
	{
		//Build download command.
		m_pipelineCommand = "https://" + server + "/" + fileGUID;

		if (maxDownloadSpeed > 0)
		{
			m_pipelineCommand += "&downloadSpeed=" + std::to_string(maxDownloadSpeed) + "&ClientToken:" + clientToken;
		}

		m_certArrayIndex = 0;
		createHttpsSrc();

		if (m_typeFind.get())
		{
			result = m_downloadBuffer.link(m_typeFind);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
		else
		{
			decodeElementsAdd(&m_downloadBuffer,
 							  pipelineType);
		}
	}

	busWatchEnable();
	registerBusCallbacks();

STI_BAIL:

	return hResult;
}

stiHResult PlaybackPipelineBase::createHttpsSrc ()
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;

	// Add http src 
	m_srcElement = GStreamerElement("souphttpsrc", SOURCE_ELEMENT_NAME);
	stiTESTCOND(m_srcElement.get (), stiRESULT_ERROR);

	m_srcElement.propertySet("ssl-ca-file", certList[m_certArrayIndex < CERT_FILE_MAX_COUNT ? m_certArrayIndex : CERT_FILE_MAX_COUNT - 1].c_str());
	m_srcElement.propertySet("ssl-strict", m_certArrayIndex < CERT_FILE_MAX_COUNT);
	m_srcElement.propertySet("location", m_pipelineCommand);
	m_playbackDecodeBin.elementAdd(m_srcElement);

	if (!m_downloadBuffer.get())
	{
		std::string downloadBufferStorage = m_playbackFilePath;
		downloadBufferStorage += PLAYBACK_TEMPLATE_BUFFER_NAME;
		
		m_downloadBuffer = GStreamerElement("downloadbuffer", "download_buffer");
		stiTESTCOND(m_downloadBuffer.get (), stiRESULT_ERROR);

		m_downloadBuffer.propertySet("max-size-bytes", (guint64)MAX_RING_BUFFER_SIZE);
		m_downloadBuffer.propertySet("low-percent", 80);
		m_downloadBuffer.propertySet("temp-template", downloadBufferStorage);
		m_playbackDecodeBin.elementAdd(m_downloadBuffer);
	}

	result = m_srcElement.link(m_downloadBuffer);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	return hResult;
}

stiHResult PlaybackPipelineBase::createDisplayBin ()
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;

	GStreamerElement queue;
	GStreamerElement videoConvert0;
	GStreamerElement videoConvert0Caps;
	GStreamerPad sinkPad;
	GStreamerElement deinterlace;
	GStreamerElement videoConvert1;
	GStreamerElement qmlSink;
	GStreamerElement fakeSink;
	GStreamerElement upload;
	GStreamerCaps gstCaps;

	// Create the display bin
	m_playbackDisplayBin = GStreamerBin("playback_display_bin");
	stiTESTCOND(m_playbackDisplayBin.get (), stiRESULT_ERROR);

	queue = GStreamerElement ("queue", "playback_display_sink_queue");
	stiTESTCOND(queue.get (), stiRESULT_ERROR);

	m_playbackDisplayBin.elementAdd(queue);

	sinkPad = queue.staticPadGet("sink");
	stiTESTCOND(sinkPad.get (), stiRESULT_ERROR);
		
	result = m_playbackDisplayBin.ghostPadAdd("sink", sinkPad);
	stiTESTCOND(result, stiRESULT_ERROR);

	videoConvert1 = GStreamerElement("videoconvert", "vid_conv_1");
	stiTESTCOND(videoConvert1.get (), stiRESULT_ERROR);

	m_playbackDisplayBin.elementAdd(videoConvert1);

	if (m_msdkFactory.get())
	{
		// Force msdkvpp to do the NV12->BGRA conversion as I
		// suspect that the NV12 stream is secretely
		// compressed and the mesa version doesn't handle the
		// compression properly
		gstCaps = GStreamerCaps ("video/x-raw(memory:DMABuf),format=BGRA");
		videoConvert1.propertySet ("caps", gstCaps);

		if (!m_playbackVideoCropBin.get())
		{
			videoConvert0 = m_msdkFactory.createElement("msdk_vpp");
			stiTESTCOND(videoConvert0.get (), stiRESULT_ERROR);

			videoConvert0Caps = GStreamerElement("capsfilter", "msdk_vpp_caps");
			stiTESTCOND(videoConvert0Caps.get(), stiRESULT_ERROR);

			gstCaps = GStreamerCaps ("video/x-raw(memory:DMABuf),format=NV12");
			stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);
			videoConvert0Caps.propertySet ("caps", gstCaps);

			m_playbackDisplayBin.elementAdd(videoConvert0);
			m_playbackDisplayBin.elementAdd(videoConvert0Caps);

			result = queue.link (videoConvert0);
			stiTESTCOND(result, stiRESULT_ERROR);

			result = videoConvert0.link (videoConvert0Caps);
			stiTESTCOND(result, stiRESULT_ERROR);

			result = videoConvert0Caps.link (videoConvert1);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
	}
	else if (m_imxFactory.get())
	{
		if (!m_playbackVideoCropBin.get())
		{
			videoConvert0 = m_imxFactory.createElement("imx_convert");
			stiTESTCOND(videoConvert0.get (), stiRESULT_ERROR);

			videoConvert0Caps = GStreamerElement("capsfilter", "imx_convert_caps");
			stiTESTCOND(videoConvert0Caps.get(), stiRESULT_ERROR);

			gstCaps = GStreamerCaps ("video/x-raw,format=RGBA");
			stiTESTCOND(gstCaps.get() != nullptr, stiRESULT_ERROR);
			videoConvert0Caps.propertySet ("caps", gstCaps);

			m_playbackDisplayBin.elementAdd(videoConvert0);
			m_playbackDisplayBin.elementAdd(videoConvert0Caps);

			result = queue.link (videoConvert0);
			stiTESTCOND(result, stiRESULT_ERROR);

			result = videoConvert0.link (videoConvert0Caps);
			stiTESTCOND(result, stiRESULT_ERROR);

			result = videoConvert0Caps.link (videoConvert1);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
	}
	else
	{
		videoConvert0 = GStreamerElement("videoconvert", "vid_conv_0");
		stiTESTCOND(videoConvert0.get (), stiRESULT_ERROR);

		m_playbackDisplayBin.elementAdd(videoConvert0);
		
		deinterlace = GStreamerElement("deinterlace", "deinterlace");
		stiTESTCOND(deinterlace.get (), stiRESULT_ERROR);

		m_playbackDisplayBin.elementAdd(deinterlace);

		result = queue.link(videoConvert0);
		stiTESTCOND(result, stiRESULT_ERROR);

		result = videoConvert0.link(deinterlace);
		stiTESTCOND(result, stiRESULT_ERROR);

		if (!m_playbackVideoCropBin.get())
		{
			result = deinterlace.link(videoConvert1);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
	}

	// If we have a videoCropBin then insert it into the display pipeline.
	if (m_playbackVideoCropBin.get())
	{
		m_playbackDisplayBin.elementAdd(m_playbackVideoCropBin);

		if (m_msdkFactory.get() || m_imxFactory.get())
		{
			result = queue.link(m_playbackVideoCropBin);
			stiTESTCOND(result, stiRESULT_ERROR);
		}
		else
		{
			result = deinterlace.link(m_playbackVideoCropBin);
			stiTESTCOND(result, stiRESULT_ERROR);
		}

		result = m_playbackVideoCropBin.link(videoConvert1);
		stiTESTCOND(result, stiRESULT_ERROR);
	}

	m_videoScale = GStreamerElement ("videoscale", "playback_video_scale");
	stiTESTCOND(m_videoScale.get (), stiRESULT_ERROR);

	m_playbackDisplayBin.elementAdd (m_videoScale);

	result = videoConvert1.link(m_videoScale);
	stiTESTCOND(result, stiRESULT_ERROR);
	
	qmlSink = GStreamerElement ("qml6glsink", "playback_qmlglsink_element");
	stiTESTCOND(qmlSink.get (), stiRESULT_ERROR);

	m_displaySink = GStreamerElement ("glsinkbin", "playback_glsinkbin_element");
	stiTESTCOND(m_displaySink.get (), stiRESULT_ERROR);

	qmlSink.propertySet ("widget", m_qquickItem);
	m_displaySink.propertySet ("sink", qmlSink);
	m_displaySink.propertySet ("force-aspect-ratio", true);
	
	// If we set this then the pipeline plays as fast as it can
	//m_displaySink.propertySet ("sync", false);

	m_playbackDisplayBin.elementAdd (m_displaySink);

	result = m_videoScale.link (m_displaySink);
	stiTESTCOND(result, stiRESULT_ERROR);

	elementAdd (m_playbackDisplayBin);

STI_BAIL:

	return hResult;
}


void PlaybackPipelineBase::streamTypeFound (
	GstElement *typeFind,
	guint  probability,
	GstCaps *caps,
	gpointer userData)
{
	auto pThis = (PlaybackPipelineBase *)userData;

	std::lock_guard<std::recursive_mutex> lock(pThis->m_execMutex);

	auto pipelineType = estiPIPELINE_TYPE_UNKNOWN;

    gchar *strType = gst_caps_to_string(caps);
	std::string streamType(strType);
	std::size_t foundH265 = streamType.find("video/x-h265");
	g_free(strType);

	// Check for h265 video stream.
	if (foundH265 != std::string::npos)
	{
		pipelineType = estiPIPELINE_TYPE_MP5;
	}
	else
	{   
		pipelineType = estiPIPELINE_TYPE_MP4;
	}

	pThis->decodeElementsAdd(&pThis->m_typeFind,
							 pipelineType);

	pThis->eventSyncDecodeBinStateWithParent();
}

void PlaybackPipelineBase::decodeElementsAdd (
	GStreamerElement *linkElement,
	EPipelineType pipelineType)
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;

	GStreamerElement capsFilter;
	GStreamerElement multiQueue;
	GStreamerElement decoder;
	GStreamerPad srcPad;

	// Check for h265 video stream.
	if (pipelineType == estiPIPELINE_TYPE_MP5)
	{
		m_parser = GStreamerElement("h265parse", "h265_parse");
		stiTESTCOND(m_parser.get (), stiRESULT_ERROR);
	}
	else
	{   
		m_parser = GStreamerElement("h264parse", "h264_parse");
		stiTESTCOND(m_parser.get (), stiRESULT_ERROR);
	}

	// The demuxer is the last thing we can add to the decode bin (we have to know the stream type).
	m_qtDemux = GStreamerElement("qtdemux", "qt_demux");
	stiTESTCOND(m_qtDemux.get (), stiRESULT_ERROR);

	g_signal_connect(m_qtDemux.get(), "pad-added", (GCallback)demuxPadAdded, this);

	capsFilter = GStreamerElement("capsfilter", "caps_filter");
	stiTESTCOND(capsFilter.get (), stiRESULT_ERROR);

	multiQueue = GStreamerElement("multiqueue", "multi_queue");
	stiTESTCOND(multiQueue.get (), stiRESULT_ERROR);

	multiQueue.propertySet("use-buffering", true);
	multiQueue.propertySet("sync-by-running-time", true);
	multiQueue.propertySet("use-interleave", true);

	if (m_msdkFactory.get())
	{
		if (pipelineType == estiPIPELINE_TYPE_MP5)
		{
			decoder = GStreamerElement("msdkh265dec", "msdk_h265_decoder");
			stiTESTCOND(decoder.get (), stiRESULT_ERROR);
		}
		else
		{
			decoder = GStreamerElement("msdkh264dec", "msdk_h264_decoder");
			stiTESTCOND(decoder.get (), stiRESULT_ERROR);
		}
	}
	else if (m_imxFactory.get())
	{
		decoder = GStreamerElement("vpudec", "vpudec");
		stiTESTCOND(decoder.get (), stiRESULT_ERROR);
	}
	else
	{
		if (pipelineType == estiPIPELINE_TYPE_MP5)
		{
			decoder = GStreamerElement("avdec_h265", "h265_decoder");
			stiTESTCOND(decoder.get (), stiRESULT_ERROR);
		}
		else
		{
			decoder = GStreamerElement("avdec_h264", "h264_decoder");
			stiTESTCOND(decoder.get (), stiRESULT_ERROR);
		}
	}

	m_playbackDecodeBin.elementAdd(m_qtDemux);  	
	m_playbackDecodeBin.elementAdd(m_parser);
	m_playbackDecodeBin.elementAdd(capsFilter);
	m_playbackDecodeBin.elementAdd(multiQueue);
	m_playbackDecodeBin.elementAdd(decoder);

	result = linkElement->link(m_qtDemux);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = m_parser.link(capsFilter);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = capsFilter.link(multiQueue);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = multiQueue.link(decoder);
	stiTESTCOND(result, stiRESULT_ERROR);

	// Create the srcpad for the decodeBin and connect it to the decoder
	srcPad = decoder.staticPadGet("src");
	stiTESTCOND(srcPad.get (), stiRESULT_ERROR);
	
	result = m_playbackDecodeBin.ghostPadAddActive("src", srcPad);
	stiTESTCOND(result, stiRESULT_ERROR);

	result = m_playbackDecodeBin.link(m_playbackDisplayBin);
	stiTESTCOND(result, stiRESULT_ERROR);

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL(g_stiVideoOutputDebug,
			vpe::trace("PlaybackPipelineBase::decodeElementsAdd(): stiIS_ERROR(hResult)\n");
		);
		
		// If we created a pipeline then clean it up.
		playbackPipelineStop ();
		
		videoScreensaverStartFailedSignal.Emit ();
	}
}


void PlaybackPipelineBase::eventSyncDecodeBinStateWithParent ()
{
	auto result = false;

	// Sync all of the elements in the pipeline to the same state
	result = m_playbackDecodeBin.syncStateWithParent();

	if (!result)
	{
		stiASSERT(false);

		// If we have a pipeline then clean it up.
		playbackPipelineStop ();
		videoScreensaverStartFailedSignal.Emit ();
	}
}


void PlaybackPipelineBase::eventSyncDisplayBinStateWithParent ()
{
	auto result = false;

	// Sync all of the elements in the pipeline to the same state
	result = m_playbackDisplayBin.syncStateWithParent();

	if (!result)
	{
		stiASSERT(false);

		// If we have a pipeline then clean it up.
		playbackPipelineStop ();
		videoScreensaverStartFailedSignal.Emit ();
	}
}


void PlaybackPipelineBase::demuxPadAdded (
	GstElement * dbin,
	GstPad * pad,
	gpointer userData)
{
	auto hResult = stiRESULT_SUCCESS;
	auto result = false;
	auto pThis = (PlaybackPipelineBase *)userData;
	GStreamerPad srcPad(pad, true);
	auto padName = srcPad.nameGet ();
	const std::string padNamePrefix {"video_"};

	stiDEBUG_TOOL (g_stiVideoOutputDebug,
		vpe::trace ("demuxPadAdded: New pad, padName = ", padName, "\n");
	);

	// Protect members from being manipulated from the PlaybackPipelineBase thread.
	std::lock_guard<std::recursive_mutex> lock(pThis->m_execMutex);

	// Make sure we have a video pad.
	if (!g_ascii_strncasecmp (padName.c_str (), padNamePrefix.c_str (), padNamePrefix.size ()))
	{
		result = pThis->m_qtDemux.link(pThis->m_parser);
		stiTESTCOND(result, stiRESULT_ERROR);

		pThis->eventSyncDecodeBinStateWithParent();
	}

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		stiDEBUG_TOOL(g_stiVideoOutputDebug,
			stiTrace("PlaybackPipelineBase::streamTypeFound: stiIS_ERROR(hResult)\n");
		);
		
		// If we created a pipeline then clean it up.
		pThis->playbackPipelineStop ();
		
		pThis->videoScreensaverStartFailedSignal.Emit ();
	}
}


void PlaybackPipelineBase::panCropRectUpdate (
	int cropLeft,
	int cropTop,
	int cropRight,
	int cropBottom)
{
	PostEvent([this, cropLeft, cropTop, cropRight, cropBottom]
	{ 
		if (m_videoCrop.get() &&
			!m_bVideoFileWaitingForShutdown)
		{
			if (m_msdkFactory.get())
			{
				m_videoCrop.propertySet("crop-left", cropLeft);
				m_videoCrop.propertySet("crop-top", cropTop);
				m_videoCrop.propertySet("crop-right", cropRight);  	 
				m_videoCrop.propertySet("crop-bottom", cropBottom);
			}
			else
			{
				m_videoCrop.propertySet("left", cropLeft);
				m_videoCrop.propertySet("top", cropTop);
				m_videoCrop.propertySet("right", cropRight);  	 
				m_videoCrop.propertySet("bottom", cropBottom);
			}
		}
	});
}


/*!
 * \brief playbackBusCallback: Call back for VideoFile bus
 *
 * \param  GstBus
 * \param  GstMessage
 * \param  gpointer - The this pointer
 * \return gboolean - Always returns true
 */
void PlaybackPipelineBase::registerBusCallbacks ()
{
	registerMessageCallback(GST_MESSAGE_EOS,
		[this](GstMessage &gstMessage)
		{
			std::lock_guard<std::recursive_mutex> lock(m_execMutex);

			stiDEBUG_TOOL(g_stiDisplayDebug,
				stiTrace("PlaybackPipelineBase::registerBusCallbacks: GST_MESSAGE_EOS\n");
			);

			if (!m_fileLoop)
			{
				videoFileEndSignal.Emit();
			}
			else if (!gst_element_seek(get(), 1.0,
						 GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
						 GST_SEEK_TYPE_SET, 0,
						 GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
			{
				stiASSERTMSG (estiFALSE, "GST_MESSAGE_EOS not handled\n");
			}
		});

	registerMessageCallback(GST_MESSAGE_SEGMENT_DONE,
		[this](GstMessage &gstMessage)
		{
			std::lock_guard<std::recursive_mutex> lock(m_execMutex);

			stiDEBUG_TOOL(g_stiDisplayDebug,
				stiTrace("PlaybackPipelineBase::registerBusCallbacks: GST_MESSAGE_SEGMENT_DONE\n");
			);

			if (m_fileLoop)
			{
				if (!gst_element_seek_simple (get(), GST_FORMAT_TIME, GST_SEEK_FLAG_SEGMENT, 0))
				{
					stiASSERTMSG (estiFALSE, "GST_MESSAGE_SEGMENT_DONE seek to frame 0 ERROR ON SEEK!\n");
				}
			}
			else
			{
				videoFileEndSignal.Emit();
			}
		});
		
	registerMessageCallback(GST_MESSAGE_STREAM_START,
		[this](GstMessage &gstMessage)
		{
			stiDEBUG_TOOL(g_stiDisplayDebug,
				stiTrace("PlaybackPipelineBase::registerBusCallbacks: GST_MESSAGE_STREAM_START\n");
			);
		});

	registerMessageCallback(GST_MESSAGE_STATE_CHANGED,
		[this](GstMessage &gstMessage)
		{
			std::lock_guard<std::recursive_mutex> lock(m_execMutex);

			if (GST_MESSAGE_SRC(&gstMessage) == GST_OBJECT(get()))
			{
				GstState oldState;
				GstState newState;
				GstState pendingState;

				gst_message_parse_state_changed (&gstMessage, &oldState, &newState, &pendingState);

				if (newState == GST_STATE_PLAYING)
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);

					auto decodeBinSrcPad = m_playbackDecodeBin.staticPadGet("src");
					if (decodeBinSrcPad.get() != nullptr)
					{
						auto decodeBinCaps = decodeBinSrcPad.currentCapsGet();
						if (decodeBinCaps.get() != nullptr)
						{
							int decodeWidth = 0;
							int decodeHeight = 0; 

							auto gstStruct = decodeBinCaps.structureGet(0);

							gstStruct.intGet("width", &decodeWidth);
							gstStruct.intGet("height", &decodeHeight);

							decodeFrameSizeUpdateSignal.Emit(decodeWidth, 
															 decodeHeight);
						}
					}
				}

				if (newState == GST_STATE_PAUSED || newState == GST_STATE_PLAYING)
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);

					if (!m_screenSaverVideoFile &&  
						m_seekNeeded)
					{
						// Perform the seek that couldn't be done earlier.
						m_seekNeeded = false;
						videoFileSeek(m_seekPos);
					}
				}

				if (oldState == GST_STATE_READY && newState == GST_STATE_PAUSED)
				{
					std::lock_guard<std::recursive_mutex> lock(m_execMutex);

					if (m_bVideoFileWaitingForShutdown)
					{
						playbackPipelineStop();
					}
					else if (m_screenSaverVideoFile)
					{
						if (!gst_element_seek_simple (get(), GST_FORMAT_TIME,
										  (GstSeekFlags)(GST_SEEK_FLAG_SEGMENT | GST_SEEK_FLAG_FLUSH), 0))
						{
							stiASSERTMSG (estiFALSE, "GST_MESSAGE_STATE_CHANGED READY->PAUSED seek to frame 0 ERROR ON SEEK!\n");
						}

						GstStateChangeReturn StateChangeReturn;
						StateChangeReturn = stateSet (GST_STATE_PLAYING);
						stiASSERTMSG (StateChangeReturn != GST_STATE_CHANGE_FAILURE, "GST_MESSAGE_STATE_CHANGED READY->PAUSED fail to to in PLAYING state!\n");
					}
				}
			}
		});

	registerMessageCallback(GST_MESSAGE_BUFFERING,
		[this](GstMessage &gstMessage)
		{
			std::lock_guard<std::recursive_mutex> lock(m_execMutex);

			GstState state = GST_STATE_VOID_PENDING;
			GstState pending = GST_STATE_VOID_PENDING;

			stateGet (&state, &pending, 0);

			gint percent = 0;
			gst_message_parse_buffering (&gstMessage, &percent);
			stiDEBUG_TOOL(g_stiDisplayDebug > 1,
				vpe::trace ("PlaybackPipelineBase::VideoFileBusCallback: Buffering (", percent, " percent done)\n");
			);

			// If we are buffered to 100 percent start playing.
			if (percent == 100 && state == GST_STATE_PAUSED && !m_fileLoop)
			{
				std::lock_guard<std::recursive_mutex> lock(m_execMutex);

				if (!m_fileLoop)
				{
					m_ringBufferingTimer.stop();
					videoFileReadyToPlaySignal.Emit();
				}
			}
		});

	registerMessageCallback(GST_MESSAGE_PROGRESS,
		[this](GstMessage &gstMessage)
		{
			GstProgressType progress = GST_PROGRESS_TYPE_ERROR;
			gst_message_parse_progress (&gstMessage, &progress, nullptr, nullptr);
			stiDEBUG_TOOL(g_stiDisplayDebug,
				stiTrace("PlaybackPipelineBase::registerBusCallbacks: Progress (%u progress done)\n", progress);
			);
		});

	registerMessageCallback(GST_MESSAGE_ASYNC_DONE,
		[this](GstMessage &gstMessage)
		{
			std::lock_guard<std::recursive_mutex> lock(m_execMutex);

			// File is loaded so we can notify app of file is ready to play.
			if (m_videoFileIsLocal)
			{
				m_videoFileIsLocal = false;
				eventUpdatePlaybackTimerTimeout();

				videoFileCreatedSignal.Emit();

				videoFileReadyToPlaySignal.Emit();
			}
		});

	registerMessageCallback(GST_MESSAGE_ERROR, 
		[this](GstMessage &gstMessage)
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_error (&gstMessage, &pError, &pDebugInfo);

			if (pError)
			{
				std::lock_guard<std::recursive_mutex> lock(m_execMutex);

				if (pError->domain == GST_RESOURCE_ERROR)
				{
					switch(pError->code)
					{
					case  GST_RESOURCE_ERROR_NOT_FOUND:
					{
						videoFileStartFailedSignal.Emit();

						// Stop the ring buffer timer so no other error is signaled.
						m_ringBufferingTimer.stop();

						// Post the stateSet() to NULL and pipeline cleanup so that we don't deadlock.
						PostEvent ([this]
						{
							// Set the pipeline to null so that it can be cleaned up.
							auto stateChangeReturn = stateSet(GST_STATE_NULL);
							stiASSERTMSG (stateChangeReturn != GST_STATE_CHANGE_FAILURE, "Failed to set pipeline state to  GST_STATE_NULL!\n");

							playbackPipelineCleanup ();
						});

						break;
					}
					case GST_RESOURCE_ERROR_OPEN_READ:
					{
						std::lock_guard<std::recursive_mutex> lock(m_execMutex);

						if (m_certArrayIndex < CERT_FILE_MAX_COUNT)
						{
							// Remove the https_src element and clean it up.
							m_srcElement.unlink(m_downloadBuffer);
							m_playbackDecodeBin.elementRemove(m_srcElement);
							auto stateChangeReturn = m_srcElement.stateSet(GST_STATE_NULL);
							stiASSERTMSG (stateChangeReturn != GST_STATE_CHANGE_FAILURE, "Fail to change souphttpsrc state to  GST_STATE_NULL!\n");

							if (stateChangeReturn == GST_STATE_CHANGE_FAILURE)
							{
								videoFileStartFailedSignal.Emit();
							}

							m_srcElement.clear();

							// Recreate the https_src element with a different cert.
							m_certArrayIndex++;
							createHttpsSrc();
							stateChangeReturn = m_srcElement.stateSet(GST_STATE_PAUSED);
							stiASSERTMSG (stateChangeReturn != GST_STATE_CHANGE_FAILURE, "Fail to change souphttpsrc state to  GST_STATE_PAUSED!\n");

							if (stateChangeReturn == GST_STATE_CHANGE_FAILURE)
							{
								videoFileStartFailedSignal.Emit();
							}
						}
						else
						{
							videoFileStartFailedSignal.Emit();
						}
						break;
					}
					}
				}
				else if (pError->domain == GST_STREAM_ERROR)
				{
					switch(pError->code)
					{
					case GST_STREAM_ERROR_FAILED:
					{
						std::string objName = GST_OBJECT_NAME(GST_MESSAGE_SRC(&gstMessage));
						if (objName != SOURCE_ELEMENT_NAME)
						{
							PostEvent ([this]
							{
								auto stateChangeReturn = stateSet (GST_STATE_NULL);
								stiASSERTMSG (stateChangeReturn != GST_STATE_CHANGE_FAILURE, "GST_STREAM_ERROR_FAILED Failed to change  state to  GST_STATE_NULL!\n");

							   if (stateChangeReturn == GST_STATE_CHANGE_FAILURE)
							   {
								   videoFileStartFailedSignal.Emit ();
								   playbackPipelineCleanup ();
							   }
							   else
							   {
								   playbackPipelineCleanup (false);
								   eventRestartVideo();
							   }
						   });
						}
						break;
					}
					}
				}

				g_error_free (pError);
				pError = nullptr;
			}

			if (pDebugInfo)
			{
				g_free (pDebugInfo);
				pDebugInfo = nullptr;
			}
		});
}


/*!
 * \brief EventRingBufferingTimerTimeout: Fires when the ring buffer is filling 
 * and we need to wait for it to fill to a point that we can begin playback of a video 
 */
void PlaybackPipelineBase::eventRingBufferingTimerTimeout ()
{
	auto hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG(hResult);

	gint percent = 0;
	gint64 estimatedDownloadTime = 0;
	gint64 position = 0;
	gint64 playLeft = -1;
	gboolean busy = false;
	GstQuery *query;

	query = gst_query_new_buffering (GST_FORMAT_TIME);

	if (gst_element_query (get (), query))
	{
		m_ringBufferTimerCount = 0;
		gst_query_parse_buffering_percent (query, &busy, &percent);
		gst_query_parse_buffering_range (query, nullptr, nullptr, nullptr, &estimatedDownloadTime);

		if (estimatedDownloadTime == -1)
		{
			estimatedDownloadTime = 0;
		}

		// Calculate the remaining playback time.
		if (!gst_element_query_position (get (), GST_FORMAT_TIME, &position))
		{
			position = -1;
		}

		if (!gst_element_query_duration (get (), GST_FORMAT_TIME, &m_videoDuration))
		{
			m_videoDuration = -1;
		}
		else
		{
			videoFileCreatedSignal.Emit ();

			// Update the current positon and video duration. (TimeScale is in nanoseconds.)
			videoFilePlayProgressSignal.Emit (0, m_videoDuration, 1000000000);
		}

		if (position != -1 && m_videoDuration != -1)
		{
			playLeft = GST_TIME_AS_MSECONDS (m_videoDuration - position);

			// Update the playback position.
			videoFileSeekReadySignal.Emit();
		}

		if ((playLeft != -1 && playLeft > estimatedDownloadTime * 1.1) || percent == 100)
		{
			videoFileReadyToPlaySignal.Emit ();
			m_updateTimer.start ();
		}
		else
		{
			m_ringBufferingTimer.start ();
		}
	}
	else if (get())
	{
		m_ringBufferTimerCount++;

		// If ringBufferTimerCount has reached MAX_RING_BUFFER_TIMER_COUNT then we have had no activity
		// for a designated amount of time and we need to error out.
		if (m_ringBufferTimerCount >= MAX_RING_BUFFER_TIMER_COUNT)
		{
			GstStateChangeReturn stateChangeReturn;
			stateChangeReturn = stateSet (GST_STATE_NULL);
			stiTESTCOND (stateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

			playbackPipelineCleanup (false);

			PostEvent([this]
			{
				eventRestartVideo();
			});
		}
		else
		{
			m_ringBufferingTimer.start ();
		}
	}

STI_BAIL:

	if (stiIS_ERROR (hResult))
	{
		videoFileStartFailedSignal.Emit ();
		playbackPipelineCleanup ();
	}
}


/*!
 * \brief eventUpdatePlaybackTimerTimeout: Updates the playing position. 
 */
void PlaybackPipelineBase::eventUpdatePlaybackTimerTimeout ()
{
	auto hResult = stiRESULT_SUCCESS;
	stiUNUSED_ARG(hResult);

	gint64 position = 0;

	stiTESTCOND(get(), stiRESULT_ERROR);

	if (!gst_element_query_position(get(), GST_FORMAT_TIME, &position))
	{
		position = -1;
	}

	if (!gst_element_query_duration(get(), GST_FORMAT_TIME, &m_videoDuration))
	{
		m_videoDuration = -1;
	}

	if (position != -1 && m_videoDuration != -1)
	{
		// Time scale is in nano seconds.
		videoFilePlayProgressSignal.Emit(position, m_videoDuration, 1000000000);
	}

	m_updateTimer.start();

STI_BAIL:;
}


void PlaybackPipelineBase::playbackPipelineStop ()
{

	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("PlaybackPipelineBase::playbackPipelineStop\n");
	);

	PostEvent([this]
	{
		auto hResult = stiRESULT_SUCCESS;
		stiUNUSED_ARG (hResult);

		if (get())
		{
			GstState state = GST_STATE_VOID_PENDING;
			GstState pending = GST_STATE_VOID_PENDING;

			stateGet (&state, &pending, 0);
			
			switch (state)
			{
			case GST_STATE_PLAYING:
			case GST_STATE_PAUSED:
			{
				stiDEBUG_TOOL(g_stiVideoOutputDebug,
					stiTrace("PlaybackPipelineBase::playbackPipelineStop: Current state PLAYING or PAUSED\n");
				);
				
				// Set to NULL state before removing
				auto StateChangeReturn = stateSet (GST_STATE_NULL);
				stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

				playbackPipelineCleanup ();

				m_bVideoFileWaitingForShutdown = false;

				break;
			}

			case GST_STATE_NULL:
			{
				stiDEBUG_TOOL(g_stiVideoOutputDebug,
					stiTrace("PlaybackPipelineBase::playbackPipelineStop: Current state NULL\n");
				);
				
				playbackPipelineCleanup ();

				m_bVideoFileWaitingForShutdown = false;
				break;
			}

			case GST_STATE_READY:
			{
				stiDEBUG_TOOL(g_stiVideoOutputDebug,
					stiTrace("PlaybackPipelineBase::playbackPipelineStop: Current state READY\n");
				);
				
				// Sometimes we lock up when going from the READY state to the NULL state so we need to
				// wait for the pipeline to transistion to the PAUSED or PLAYINNG state before we shut it down.
				m_bVideoFileWaitingForShutdown = true;

				if (pending != GST_STATE_PAUSED &&
					pending != GST_STATE_PLAYING)
				{
					// In case the pipeline is just waiting in the READY state move it to the PAUSED state
					// so it can shutdown. 
					auto StateChangeReturn = stateSet(GST_STATE_PAUSED);
					stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);
				}
				break;
			}

			case GST_STATE_VOID_PENDING:
				stiDEBUG_TOOL(g_stiVideoOutputDebug,
					stiTrace("PlaybackPipelineBase::playbackPipelineStop: Current state VOID_PENDING\n");
				);
				break;
			}
		}
		else
		{
			stiDEBUG_TOOL(g_stiVideoOutputDebug,
				stiTrace("PlaybackPipelineBase::playbackPipelineStop: No pipeline\n");
			);
		}

STI_BAIL:;
	});
}


void PlaybackPipelineBase::playbackPipelineCleanup (
	bool fileClosed)
{
	stiDEBUG_TOOL(g_stiVideoOutputDebug,
		stiTrace("PlaybackPipelineBase::playbackPipelineCleanup\n");
	);
	
	// Stop the timers.
	m_ringBufferingTimer.stop ();
	m_updateTimer.stop ();

	clear();

	m_playbackVideoCropBin.clear();
	m_msdkFactory.clear();
	m_imxFactory.clear();
	m_typeFind.clear();
	m_downloadBuffer.clear();

	if (fileClosed)
	{
		// Reset the decode frame size.
		decodeFrameSizeUpdateSignal.Emit(CstiVideoOutputBase::DEFAULT_DECODE_WIDTH, 
										 CstiVideoOutputBase::DEFAULT_DECODE_HEIGHT);

		// Signal that the file is closed.
		videoFileClosedSignal.Emit (m_fileGUID);

		m_server.clear ();
		m_fileGUID.clear ();
		m_clientToken.clear ();
		m_maxDownloadSpeed = 0;
		m_fileLoop = false;
		m_screenSaverPathName.clear ();
	}

	m_ringBufferTimerCount = 0;
}


void PlaybackPipelineBase::videoFilePlay (
	EPlayRate speed)
{
	PostEvent([this, speed] 
	{
		auto hResult = stiRESULT_SUCCESS;
		GstStateChangeReturn StateChangeReturn = GST_STATE_CHANGE_SUCCESS;
		GstState currentState = GST_STATE_NULL;
		GstState pendingState = GST_STATE_NULL;
		GStreamerPad displayBinPad;

		stiTESTCOND(get(), stiRESULT_ERROR);

		// Get the pipeline state.
		StateChangeReturn = stateGet (&currentState,
									  &pendingState,
									  GST_CLOCK_TIME_NONE);

		if (speed != ePLAY_PAUSED)
		{
			StateChangeReturn = stateSet(GST_STATE_PLAYING);
			stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

			gdouble playRate = 1.0;
			if (speed == ePLAY_SLOW)
			{
				playRate = 0.5;
			}
			else if (speed == ePLAY_FAST)
			{
				playRate = 2.0;
			}

#if DEVICE == DEV_X86
			// If we don't set the position of the file while changing playback rate,
			// the rate always changes to 2x.
			gint64 position = 0;
			gst_element_query_position(get(), GST_FORMAT_TIME, &position);

			gst_element_seek(get(), playRate, 
					GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
					GST_SEEK_TYPE_SET, position,
					GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
#else
			gst_element_seek(get(), playRate, 
				   GST_FORMAT_TIME, GST_SEEK_FLAG_INSTANT_RATE_CHANGE,
				   GST_SEEK_TYPE_NONE, 0,
				   GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
#endif
			m_updateTimer.start ();
		}
		else if (currentState != GST_STATE_PAUSED && speed == ePLAY_PAUSED)
		{
			StateChangeReturn = stateSet(GST_STATE_PAUSED);
			stiTESTCOND(StateChangeReturn != GST_STATE_CHANGE_FAILURE, stiRESULT_ERROR);

			m_updateTimer.stop ();
		}

	STI_BAIL:

		if (stiIS_ERROR(hResult))
		{
			stiASSERTMSG (estiFALSE, "Error in PlaybackPipelineBase::videoFilePlay\n");
		}
	});
}


void PlaybackPipelineBase::videoFileSeek (
	gint64 seconds)
{
	PostEvent([this, seconds]
	{
		auto hResult = stiRESULT_SUCCESS;
		stiUNUSED_ARG (hResult);

		auto result = false;
		GstState state = GST_STATE_VOID_PENDING;
		GstState pending = GST_STATE_VOID_PENDING;
		gint64 position = 0;

		stateGet (&state, &pending, 0);

		if (state == GST_STATE_PLAYING || state == GST_STATE_PAUSED)
		{
			// Check for the position and if we are skipping to within 1 second of the position, don't skip.
			// This prevents an error in the pipeline that occurs if we skip to 0 while trying to start up the pipeline.
			gst_element_query_position(get(), GST_FORMAT_TIME, &position);
			if (abs((seconds * 1000000000) - position) > 1000000000)
			{
				result = gst_element_seek(get(), 1.0, 
								 GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
								 GST_SEEK_TYPE_SET, seconds * 1000000000,
								 GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);
				stiTESTCOND(result, stiRESULT_ERROR);

				// The seek was successful so update the playback postion. This is
				// needed if playback is in the paused state.
				eventUpdatePlaybackTimerTimeout();
			}
		}
		else
		{
			// We need to be in the paussed or playing state to seek.  So set it up to seek
			// when we enter the paused or playing state.
			m_seekNeeded = true;
			m_seekPos = seconds;
		}

	STI_BAIL:;
	});
}

void PlaybackPipelineBase::eventRestartVideo()
{
	m_videoFileLoadStarting = false;

	if (!m_screenSaverPathName.empty())
	{
		screensaverLoad(m_screenSaverPathName,
						m_cropRight,
						m_cropBottom);
	}
	else
	{
		videoFileLoad(m_server,
					  m_fileGUID,
					  m_clientToken,
					  m_maxDownloadSpeed,
					  m_fileLoop);
	}
}

void PlaybackPipelineBase::playbackViewWidgetSet (
		void *qquickItem)
{
	PostEvent (
		[this, qquickItem]
		{
			stiDEBUG_TOOL (g_stiVideoOutputDebug,
				vpe::trace (__PRETTY_FUNCTION__, " setting QQuickItem =",  qquickItem, " \n");
			);

			if (m_qquickItem != qquickItem)
			{
				m_qquickItem = qquickItem;

				if (m_videoFileLoadStarting)
				{
					eventRestartVideo();
				}
			}
		});
}

#if 0
int bufferCount = 0;
int totalBuffers = 0;
GstPadProbeReturn PlaybackPipelineBase::DebugProbeFunction (
	GstPad * gstPad,
	GstPadProbeInfo *info,
	gpointer userData)
{
	auto pThis = static_cast<PlaybackPipelineBase *>(userData);

	pThis->writeGraph("playbackPipelineBase-hw");
#if 0
	if (GST_PAD_IS_SRC(gstPad))
	{
		bufferCount--;
		vpe::trace(__PRETTY_FUNCTION__, " SRC pad has buffer: ", info->id, " bufferCount: ", bufferCount, " ***********************\n");
	}
	else
	{
		bufferCount++;
		totalBuffers++;
		vpe::trace(__PRETTY_FUNCTION__, " SINK pad has buffer: ", info->id, " bufferCount: ", bufferCount, " totalBufCount: ", totalBuffers, " ***********************\n");
	}


	if (gst_pad_is_blocking(gstPad))
	{
		vpe::trace(__PRETTY_FUNCTION__, "pad is blocking\n");
	}
#endif
#if 0
	GstBuffer * pGstBuffer;
	
	pGstBuffer = GST_PAD_PROBE_INFO_BUFFER (info);

	stiDEBUG_TOOL (g_stiVideoOutputDebug > 2,
		GstMapInfo gstMapInfo;
		gst_buffer_map (pGstBuffer, &gstMapInfo, (GstMapFlags) GST_MAP_READ);
		stiTrace ("DebugProbeFunction: buffer %p, pushing, timestamp %"
				GST_TIME_FORMAT ", duration %" GST_TIME_FORMAT
				", offset %" G_GINT64_FORMAT ", offset_end %" G_GINT64_FORMAT
				", size %d, flags 0x%x\n",
				pGstBuffer,
				GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (pGstBuffer)),
				GST_TIME_ARGS (GST_BUFFER_DURATION (pGstBuffer)),
				gstMapInfo.offset, GST_BUFFER_OFFSET_END (pGstBuffer),
				gstMapInfo.size, GST_BUFFER_FLAGS (pGstBuffer));
		gst_buffer_unmap (pGstBuffer, &gstMapInfo);
	);
#endif
	return GST_PAD_PROBE_OK;
}
#endif


