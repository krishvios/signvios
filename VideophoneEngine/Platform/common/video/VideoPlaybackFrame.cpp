#include "VideoPlaybackFrame.h"


VideoPlaybackFrame::VideoPlaybackFrame(size_t initialSize)
{
	m_buffer.resize (initialSize);
}


uint8_t *VideoPlaybackFrame::BufferGet()
{
	return m_buffer.data();
}


uint32_t VideoPlaybackFrame::BufferSizeGet () const
{
	return m_buffer.size();
}


stiHResult VideoPlaybackFrame::BufferSizeSet (uint32_t bufferSize)
{
	stiHResult hResult = stiRESULT_SUCCESS;
	
	m_buffer.resize (bufferSize);

	return hResult;
}


uint32_t VideoPlaybackFrame::DataSizeGet () const
{
	return m_dataSize;
}


stiHResult VideoPlaybackFrame::DataSizeSet (uint32_t dataSize)
{
	auto hResult = stiRESULT_SUCCESS;
	if (m_dataSize > m_buffer.size())
	{
		hResult = BufferSizeSet (dataSize);
		stiTESTRESULT ();
	}
	m_dataSize = dataSize;

STI_BAIL:
	return hResult;
}


bool VideoPlaybackFrame::FrameIsCompleteGet () 
{ 
	return m_isComplete;
}


void VideoPlaybackFrame::FrameIsCompleteSet (bool bFrameIsComplete)
{ 
	m_isComplete = bFrameIsComplete;
}


void VideoPlaybackFrame::FrameIsKeyframeSet (bool bFrameIsKeyframe)
{ 
	m_isKeyframe = bFrameIsKeyframe;
}


bool VideoPlaybackFrame::FrameIsKeyframeGet ()
{
	return m_isKeyframe;
}
