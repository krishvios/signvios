#pragma once

#include "VideoPlaybackFrame.h"


class CstiVideoOutputBase;


class CstiVideoPlaybackFrame : public VideoPlaybackFrame
{
public:

	CstiVideoPlaybackFrame(int playbackBufferSize)
	:
		VideoPlaybackFrame (playbackBufferSize)
	{
	}

	CstiVideoPlaybackFrame (const CstiVideoPlaybackFrame &other) = delete;
	CstiVideoPlaybackFrame (CstiVideoPlaybackFrame &&other) = delete;
	CstiVideoPlaybackFrame &operator= (const CstiVideoPlaybackFrame &other) = delete;
	CstiVideoPlaybackFrame &operator= (CstiVideoPlaybackFrame &&other) = delete;

	~CstiVideoPlaybackFrame () override = default;

	CstiVideoOutputBase *VideoOutputGet ()
	{
		return(m_videoOutput);
	}

	void VideoOutputSet (
		CstiVideoOutputBase *videoOutput)
	{
		m_videoOutput = videoOutput;
	}

private:

	CstiVideoOutputBase *m_videoOutput {nullptr};
};


