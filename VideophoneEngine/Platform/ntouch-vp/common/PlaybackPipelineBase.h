#pragma once

#include "IVideoOutputVP.h"
#include <gst/gst.h>
#include "stiDefs.h"
#include "stiSVX.h"
#include "CstiEventQueue.h"
#include "CstiSignal.h"
#include "CstiTimer.h"
#include "GStreamerElement.h"
#include "GStreamerElementFactory.h"
#include "MediaPipeline.h"

class PlaybackPipelineBase : public CstiEventQueue, public MediaPipeline
{
public:
	PlaybackPipelineBase(const std::string &name);
	~PlaybackPipelineBase();

	PlaybackPipelineBase (const PlaybackPipelineBase &other) = delete;
	PlaybackPipelineBase (PlaybackPipelineBase &&other) = delete;
	PlaybackPipelineBase &operator= (const PlaybackPipelineBase &other) = delete;
	PlaybackPipelineBase &operator= (PlaybackPipelineBase &&other) = delete;

	void create();

	stiHResult videoFileLoad(
		const std::string &server,
		const std::string &fileGUID,
		const std::string &clientToken,
		int maxDownloadSpeed,
		bool videoLoop);

	stiHResult screensaverLoad(
		const std::string &filePathName,
		int cropRight,
		int cropBottom);

	void playbackPipelineStop();

	void videoFileSeek(
		gint64 seconds);

	void videoFilePlay(
		EPlayRate speed);

	void panCropRectUpdate(
		int cropLeft,
		int cropTop,
		int cropRight,
		int cropBottom);

	void playbackViewWidgetSet (
		void *qquickItem);

	CstiSignal<> videoFileStartFailedSignal;
	CstiSignal<> videoFileCreatedSignal;
	CstiSignal<> videoFileReadyToPlaySignal;
	CstiSignal<> videoFileEndSignal;
	CstiSignal<const std::string> videoFileClosedSignal;
	CstiSignal<uint64_t, uint64_t, uint64_t> videoFilePlayProgressSignal;
	CstiSignal<> videoFileSeekReadySignal;
	CstiSignal<> videoScreensaverStartFailedSignal;
	CstiSignal<uint32_t, uint32_t> decodeFrameSizeUpdateSignal; 

protected:

	static constexpr const int BUFFER_RETURN_ERROR_TIMEOUT = 5000; //5 seconds
	static constexpr const int RING_BUFFERING_TIMEOUT = 50; //.05 seconds
	static constexpr const int UPDATE_PLAYBACK_TIMEOUT = 250; //.25 seconds

	CstiSignalConnection::Collection m_signalConnections;

private:

	enum EPipelineType
	{
		estiPIPELINE_TYPE_UNKNOWN,
		estiPIPELINE_TYPE_MP4,
		estiPIPELINE_TYPE_MP5
	};

	void eventRestartVideo();

	stiHResult playbackPipelineCreate(
		std::string server,
		std::string fileGUID,
		std::string clientToken,
		int maxDownloadSpeed);

	void playbackPipelineCleanup (
		bool fileClosed = true);

	stiHResult createDisplayBin ();

	static void streamTypeFound(
		GstElement *typeFind,
		guint  probability,
		GstCaps *caps,
		gpointer data);

	static void demuxPadAdded (
		GstElement * dbin,
		GstPad * pad,
		gpointer userData);

	static gboolean playbackBusCallback(
		GstBus *pPipelineBus,
		GstMessage *pMsg,
		gpointer data);

	void decodeElementsAdd(
		GStreamerElement *linkElement,
		EPipelineType pipelineType);

	stiHResult createHttpsSrc ();

#if 0
	static GstPadProbeReturn DebugProbeFunction (
		GstPad * gstPad,
		GstPadProbeInfo *info,
		gpointer userData);
#endif

	void eventRingBufferingTimerTimeout ();
	void eventUpdatePlaybackTimerTimeout ();
	void eventSyncDecodeBinStateWithParent();
	void eventSyncDisplayBinStateWithParent();

	void registerBusCallbacks ();

	void *m_qquickItem {nullptr};

	GStreamerElementFactory m_msdkFactory;
	GStreamerElementFactory m_imxFactory;
	GStreamerBin m_playbackDecodeBin;
	GStreamerBin m_playbackDisplayBin;
	GStreamerBin m_playbackVideoCropBin;
	GStreamerElement m_typeFind;
	GStreamerElement m_qtDemux;
	GStreamerElement m_parser;
	GStreamerElement m_videoCrop;
	GStreamerElement m_videoCropCaps;
	GStreamerElement m_downloadBuffer;
	GStreamerElement m_srcElement;

	GStreamerElement m_displaySink;
	GStreamerElement m_videoScale;
	GStreamerPad m_displaySinkPad;

	bool m_bVideoFileWaitingForShutdown {false};
	bool m_screenSaverVideoFile {false};
	bool m_fileLoop {false};
	bool m_videoFileIsLocal {false};
	bool m_seekNeeded {false};
	
	gint64 m_videoDuration {0};
	gint64 m_seekPos {0};
	vpe::EventTimer m_ringBufferingTimer;
	vpe::EventTimer m_updateTimer;
	int m_ringBufferTimerCount {0};

	std::string m_playbackFilePath;
	std::string m_pipelineCommand;
	bool m_videoFileLoadStarting {false};
	std::string m_screenSaverPathName;
	int m_cropRight {0};
	int m_cropBottom {0};
	std::string m_server;
	std::string m_fileGUID;
	std::string m_clientToken;
	int m_maxDownloadSpeed {0};
	int m_certArrayIndex {0};
};
