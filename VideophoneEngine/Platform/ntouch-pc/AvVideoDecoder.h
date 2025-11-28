#pragma once
#include "IVideoDecoder.h"
#include <map>
#include "AvWorkerFrame.h"
#include <queue>
#include <mutex>

class AvVideoDecoder :
	public IVideoDecoder
{
public:
	AvVideoDecoder ();
	~AvVideoDecoder ();
	bool decode (IstiVideoPlaybackFrame *frame, uint8_t *flags) override;
	std::shared_ptr<IVideoDisplayFrame> frameGet () override;
	void frameReturn (std::shared_ptr<IVideoDisplayFrame>) override;
	bool codecSet (EstiVideoCodec codec) override;
	void clear () override;
	std::string decoderDescription () override;
	static vpe::DeviceSupport supportGet (EstiVideoCodec codec, uint16_t profile, uint16_t level);

private:
	const int StartingWorkerFrameCount = 5;
	int outputFrameCount = StartingWorkerFrameCount;

	std::recursive_mutex m_outputFramesLock;
	std::recursive_mutex m_workFramesLock;

	std::map<AVCodecID, AVCodec *> m_codec;
	std::map<AVCodecID, AVCodecContext *> m_codecContext;
	AVCodecID m_currentCodecId = AV_CODEC_ID_H264;
	AVCodecID m_supportedCodecs[2] =
	{
		AV_CODEC_ID_H264,
		AV_CODEC_ID_HEVC,
	};

	std::queue<std::shared_ptr<AvWorkerFrame>> m_outputFrames;
	std::queue<std::shared_ptr<AvWorkerFrame>> m_workFrames;

	int m_consecutiveFailures = 0;
};

