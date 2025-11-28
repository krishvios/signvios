#pragma once
#include "IstiVideoPlaybackFrame.h"
#include <vector>

class VideoPlaybackFrame : public IstiVideoPlaybackFrame
{
public:
	VideoPlaybackFrame (size_t initialSize);

	~VideoPlaybackFrame () override = default;

	VideoPlaybackFrame (const VideoPlaybackFrame &other) = delete;
	VideoPlaybackFrame (VideoPlaybackFrame &&other) = delete;
	VideoPlaybackFrame &operator= (const VideoPlaybackFrame &other) = delete;
	VideoPlaybackFrame &operator= (VideoPlaybackFrame &&other) = delete;

	// pointer to the video packet data
	uint8_t *BufferGet () override;

	// size of this buffer in bytes
	uint32_t BufferSizeGet () const override;

	stiHResult BufferSizeSet (uint32_t bufferSize) override;

	// Number of bytes in the buffer.
	uint32_t DataSizeGet () const override;

	stiHResult DataSizeSet (uint32_t dataSize) override;

	bool FrameIsCompleteGet () override;
	void FrameIsCompleteSet (bool isComplete) override;
	void FrameIsKeyframeSet (bool isKeyframe) override;
	bool FrameIsKeyframeGet () override;

private:
	std::vector<uint8_t> m_buffer;
	uint32_t m_dataSize = 0;
	bool m_isComplete = false;
	bool m_isKeyframe = false;
};
