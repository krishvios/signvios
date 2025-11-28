#include "AvVideoDecoder.h"


AvVideoDecoder::AvVideoDecoder ()
{
	avcodec_register_all ();

	for (AVCodecID codec : m_supportedCodecs)
	{
		m_codec[codec] = avcodec_find_decoder (codec);
		m_codecContext[codec] = avcodec_alloc_context3 (m_codec[codec]);

		m_codecContext[codec]->err_recognition = true;
		m_codecContext[codec]->flags |= AV_CODEC_FLAG_PSNR;
		m_codecContext[codec]->skip_frame = AVDISCARD_NONREF;
		m_codecContext[codec]->refcounted_frames = 1;

		int result = avcodec_open2 (m_codecContext[codec], m_codec[codec], NULL);
		stiTrace ("AvVideoDecoder opened codec %s, result = %d", m_codec[codec]->name, result);
	}

	for (int i = 0; i < outputFrameCount; i++)
	{
		m_workFrames.push (std::make_shared<AvWorkerFrame> ());
	}
}


AvVideoDecoder::~AvVideoDecoder ()
{
	for (auto codec : m_supportedCodecs)
	{
		auto context = m_codecContext[codec];
		if (context)
		{
			avcodec_close (context);
			av_free (context);
		}
	}
}


bool AvVideoDecoder::decode (IstiVideoPlaybackFrame *frame, uint8_t *flags)
{
	auto codecContext = m_codecContext[m_currentCodecId];
	AVPacket packet;
	av_init_packet (&packet);
	packet.data = frame->BufferGet ();
	packet.size = frame->DataSizeGet ();

	int resultSend = avcodec_send_packet (codecContext, &packet);
	if (resultSend == 0 || resultSend == AVERROR (EAGAIN))
	{
		std::shared_ptr<AvWorkerFrame> workFrame = nullptr;
		{
			const int MaxFrameRetries = 30;

			for (auto i = 0; i < MaxFrameRetries; i++)
			{
				std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
				if (m_workFrames.empty ())
				{
					Sleep (10);
					continue;
				}
				else
				{
					workFrame = m_workFrames.front ();
					m_workFrames.pop ();
					break;
				}
			}
		}
		if (workFrame != nullptr)
		{
			workFrame->frameReset ();
			int resultReceive = avcodec_receive_frame (codecContext, workFrame->m_frame);
			if (!resultReceive)
			{	// Success
				m_consecutiveFailures = 0;
				if (workFrame->m_frame->key_frame)
				{
					// signal is a key frame
					*flags |= IstiVideoDecoder::FLAG_KEYFRAME;
				}
				*flags |= IstiVideoDecoder::FLAG_FRAME_COMPLETE;
				std::lock_guard<std::recursive_mutex> lock (m_outputFramesLock);
				m_outputFrames.push (workFrame);
				if (resultSend == AVERROR (EAGAIN))
				{
					*flags |= IstiVideoDecoder::FLAG_RESEND_FRAME;
				}
				return true;
			}
			frameReturn (workFrame);
		}
	}

	*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
	m_consecutiveFailures++;

#define MAX_CONSECUTIVE_FAILS 50 // just a wild guess
	if (m_consecutiveFailures > MAX_CONSECUTIVE_FAILS)
	{
		// decoder can abort under certain circumstances where we keep feeding it data
		// but it hasn't decoded any frames.
		*flags |= IstiVideoDecoder::FLAG_IFRAME_REQUEST;
		m_consecutiveFailures = 0;
		avcodec_close (m_codecContext[m_currentCodecId]);
		if (avcodec_open2 (m_codecContext[m_currentCodecId], m_codec[m_currentCodecId], NULL) < 0)
		{
			stiTrace ("AvVideoDecoder::decode Failed to recreate decoder\n");
		}
	}

	return false;
}

std::shared_ptr<IVideoDisplayFrame> AvVideoDecoder::frameGet ()
{
	std::shared_ptr<IVideoDisplayFrame> outputFrame;
	{
		std::lock_guard<std::recursive_mutex> lock (m_outputFramesLock);
		if (!m_outputFrames.empty ())
		{
			outputFrame = m_outputFrames.front ();
			m_outputFrames.pop ();
		}
	}
	return outputFrame;
}

void AvVideoDecoder::frameReturn (std::shared_ptr<IVideoDisplayFrame> returnFrame)
{
	std::lock_guard<std::recursive_mutex> lock (m_workFramesLock);
	m_workFrames.push (std::dynamic_pointer_cast<AvWorkerFrame>(returnFrame));
}


bool AvVideoDecoder::codecSet (EstiVideoCodec codec)
{
	switch (codec)
	{
		case estiVIDEO_H264:
			m_currentCodecId = AV_CODEC_ID_H264;
			break;

		case estiVIDEO_H265:
			m_currentCodecId = AV_CODEC_ID_HEVC;
			break;

		default:
			stiAssert (false, "AvVideoDecoder::codecSet Invalid codec set (%i)\n", (int)codec);
			return false;
	}

	return true;
}


void AvVideoDecoder::clear ()
{
	for (auto codec : m_supportedCodecs)
	{
		avcodec_flush_buffers (m_codecContext[codec]);
	}
	std::lock (m_workFramesLock, m_outputFramesLock);
	while (!m_outputFrames.empty ())
	{
		m_workFrames.push (m_outputFrames.front ());
		m_outputFrames.pop ();
	}
	m_workFramesLock.unlock ();
	m_outputFramesLock.unlock ();
}

std::string AvVideoDecoder::decoderDescription ()
{
	std::string avCodecDescr = "AVCodec_";
	if (!m_codec[m_currentCodecId])
	{
		return avCodecDescr + "Uninitialized";
	}
	auto codecName = m_codec[m_currentCodecId]->name;
	if (!codecName)
	{
		return avCodecDescr + "NotSet";
	}
	return avCodecDescr + codecName;
}

vpe::DeviceSupport AvVideoDecoder::supportGet (EstiVideoCodec codec, uint16_t profile, uint16_t level)
{
	switch (codec)
	{
		case estiVIDEO_H264:
			return vpe::DeviceSupport::Software;
		case estiVIDEO_H265:
			return vpe::DeviceSupport::Software;
		default:
			return vpe::DeviceSupport::Unavailable;
	}
}
