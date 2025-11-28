#include "AvWorkerFrame.h"

AvWorkerFrame::AvWorkerFrame ()
{
	m_frame = av_frame_alloc ();
}

AvWorkerFrame::~AvWorkerFrame ()
{
	if (m_frame)
	{
		av_free (m_frame);
		m_frame = nullptr;
	}
}

std::tuple<uint8_t *, uint8_t *, uint8_t *> AvWorkerFrame::data ()
{
	return std::tuple<uint8_t *, uint8_t *, uint8_t *> (m_frame->data[0], m_frame->data[1], m_frame->data[2]);
}

std::tuple<int, int, int> AvWorkerFrame::stride ()
{
	return std::tuple<int, int, int> (m_frame->linesize[0], m_frame->linesize[1], m_frame->linesize[2]);
}

int AvWorkerFrame::widthGet () const
{
	return m_frame->width;
}

int AvWorkerFrame::heightGet () const
{
	return m_frame->height;
}

EstiColorSpace AvWorkerFrame::colorspaceGet ()
{
	return EstiColorSpace::estiCOLOR_SPACE_I420;
}

void AvWorkerFrame::frameReset()
{
	m_frame->width = 0;
	m_frame->height = 0;
}
