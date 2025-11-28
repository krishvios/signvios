#pragma once

#include "IVideoOutputVP.h"
#include "CstiEventQueue.h"
#include "GStreamerElement.h"
#include "GStreamerPipeline.h"
#include "GStreamerBuffer.h"
#include "CFifo.h"
#include "CstiVideoPlaybackFrame.h"
#include "CstiTimer.h"

class CstiVideoInput;
class CstiCECControl;


class CstiVideoOutputBase : public CstiEventQueue, public IVideoOutputVP
{
public:

	CstiVideoOutputBase (
		int playbackBufferSize,
		int numPaybackBuffers);

	~CstiVideoOutputBase () override;

	CstiVideoOutputBase (const CstiVideoOutputBase &other) = delete;
	CstiVideoOutputBase (CstiVideoOutputBase &&other) = delete;
	CstiVideoOutputBase &operator= (const CstiVideoOutputBase &other) = delete;
	CstiVideoOutputBase &operator= (CstiVideoOutputBase &&other) = delete;

	virtual stiHResult Startup ();

	virtual void DisplaySettingsGet (
		SstiDisplaySettings * pDisplaySettings) const override;
		
	stiHResult DisplaySettingsSet (
		const SstiDisplaySettings *pDisplaySettings) override;
	
	virtual stiHResult OutputImageSettingsPrint (
		const IstiVideoOutput::SstiImageSettings *pImageSettings);
	
	virtual bool ImageSettingsEqual (
		const IstiVideoOutput::SstiImageSettings *pImageSettings0,
		const IstiVideoOutput::SstiImageSettings *pImageSettings1);

	stiHResult VideoRecordSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const override;

	stiHResult VideoPlaybackSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const override;

	stiHResult videoFilePlay (
		EPlayRate speed) override
	{
		return stiRESULT_ERROR;
	}

	stiHResult videoFileSeek (
		uint32_t seconds) override
	{
		return stiRESULT_ERROR;
	}

	stiHResult VideoFilePlay (
		const std::string &filePath) override
	{
		return stiRESULT_ERROR;
	}

	stiHResult videoFileStop () override
	{
		return stiRESULT_ERROR;
	}

	void RemoteViewWidgetSet (
		void *qquickItem) override {};
		
   	void playbackViewWidgetSet (
		void *qquickItem) override {};

	ISignal<>& videoFileStartFailedSignalGet () override;
	ISignal<>& videoFileCreatedSignalGet () override;
	ISignal<>& videoFileReadyToPlaySignalGet () override;
	ISignal<>& videoFileEndSignalGet () override;
	ISignal<const std::string>& videoFileClosedSignalGet () override;
	ISignal<uint64_t, uint64_t, uint64_t>& videoFilePlayProgressSignalGet () override;
	ISignal<>& videoFileSeekReadySignalGet () override;

	stiHResult VideoPlaybackFrameGet (
		IstiVideoPlaybackFrame **ppVideoFrame) override;

	stiHResult VideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame) override;

	stiHResult VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame) override;

	int GetDeviceFD () const override;

	bool videoOutputRunning () override
	{
		std::lock_guard<std::recursive_mutex> lock(m_execMutex);
		return m_bPlaying;
	}

public:
	static constexpr int DEFAULT_DECODE_WIDTH = 1920;
	static constexpr int DEFAULT_DECODE_HEIGHT = 1080;

protected:

	static constexpr int NUM_DEC_BITSTREAM_BUFS = 15;
	static constexpr const int BUFFER_RETURN_ERROR_TIMEOUT = 5000; //5 seconds
	static constexpr const int VIDEO_FILE_PAN_PORTAL_WIDTH = 1536;
	static constexpr const int VIDEO_FILE_PAN_PORTAL_HEIGHT = 864;
	static constexpr const int VIDEO_FILE_PAN_WIDTH = 1920;
	static constexpr const int VIDEO_FILE_PAN_HEIGHT = 1080;

	uint32_t m_un32DecodeWidth {0};
	uint32_t m_un32DecodeHeight {0};

	enum EVideoFilePortalDirection
	{
		eMOVE_LEFT,
		eMOVE_RIGHT,
		eMOVE_UP,
		eMOVE_DOWN
	};

	int m_nVideoFilePortalX {0};
	int m_nVideoFilePortalY {0};
	EVideoFilePortalDirection m_eVideoFilePortalDirection {eMOVE_RIGHT};

	virtual stiHResult DecodeDisplaySinkMove () = 0;

	SstiDisplaySettings m_DisplaySettings {};
	bool m_bHidingRemoteView {false};

	bool m_bPlaying {false};
	CstiVideoInput *m_pVideoInput {nullptr};

	std::shared_ptr<CstiCECControl> m_cecControl;

	CstiSignal<> videoFileStartFailedSignal;
	CstiSignal<> videoFileCreatedSignal;
	CstiSignal<> videoFileReadyToPlaySignal;
	CstiSignal<> videoFileEndSignal;
	CstiSignal<const std::string> videoFileClosedSignal;
	CstiSignal<uint64_t, uint64_t, uint64_t> videoFilePlayProgressSignal;
	CstiSignal<> videoFileSeekReadySignal;

	stiHResult RemoteViewPrivacySet (
		EstiBool bPrivacy) override;

	stiHResult RemoteViewHoldSet (
		EstiBool bHold) override;

	virtual stiHResult VideoSinkVisibilityUpdate (
		bool bClearReferenceFrame)
	{
		return stiRESULT_SUCCESS;
	};

	bool m_bPrivacy {false};
	bool m_bHolding {false};
	bool m_bSPSAndPPSSentToDecoder {false};

	void eventVideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame);
	void eventFilePanTimerTimeout ();
	
	virtual void panCropRectUpdate () = 0;

	stiHResult VideoFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame);

	CFifo<CstiVideoPlaybackFrame> m_oBitstreamFifo;
	std::list<IstiVideoPlaybackFrame *> m_FramesInGstreamer;
	std::list<IstiVideoPlaybackFrame *> m_FramesInApplication;

	STI_OS_SIGNAL_TYPE m_Signal {nullptr};

	bool m_lastBitsreamFifoGetSuccesful {true};

	unsigned int m_maxFramesInGstreamer {0};
	unsigned int m_maxFramesInApplication {0};

	bool m_bFirstKeyframeRecvd {false};

	EstiVideoCodec m_eCodec {estiVIDEO_H264};

	CstiTimer m_bufferReturnErrorTimer;
	CstiTimer m_videoFilePanTimer;

	stiHResult DecodePipelineBlackFramePush ();
	stiHResult H264DecodePipelineBlackFramePush ();
	stiHResult H263DecodePipelineBlackFramePush ();

	CstiSignalConnection::Collection m_signalConnections;

private:
	static constexpr const int VIDEO_FILE_PAN_SHIFT_TIMEOUT = 15000; //15 seconds

	void GstreamerVideoFrameDiscardCallback (
		IstiVideoPlaybackFrame *videoFrame);

	virtual void EventGstreamerVideoFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame) = 0;

	virtual GstFlowReturn pushBuffer (
		GStreamerBuffer &gstBuffer) = 0;
		
	virtual void applySPSFixups (
		uint8_t *spsNalUnit);
};

GstState ChangeStateWait (
	GStreamerElement pipeline,
	GstClockTime timeout = 5000 * GST_MSECOND);

