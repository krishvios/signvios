#include "IntelWorkerFrame.h"

IntelWorkerFrame::IntelWorkerFrame () :
	m_surfaceBuffer (FrameSize)
{
	m_frame = new mfxFrameSurface1 ();
	memset (m_frame, 0, sizeof (mfxFrameSurface1));
	m_frame->Data.Y = m_surfaceBuffer.data ();
	m_frame->Data.UV = m_frame->Data.Y + MaxFrameWidth * MaxFrameHeight;
}

IntelWorkerFrame::~IntelWorkerFrame ()
{
	delete m_frame;
}

std::tuple<uint8_t *, uint8_t *, uint8_t *> IntelWorkerFrame::data ()
{
	return { m_frame->Data.Y, m_frame->Data.UV, nullptr };
}

std::tuple<int, int, int> IntelWorkerFrame::stride ()
{
	return { m_frame->Data.Pitch, m_frame->Data.Pitch, 0 };
}

int IntelWorkerFrame::widthGet () const
{
	return m_frame->Info.Width;
}

int IntelWorkerFrame::heightGet () const
{
	return m_frame->Info.Height;
}

EstiColorSpace IntelWorkerFrame::colorspaceGet ()
{
	
	return EstiColorSpace::estiCOLOR_SPACE_NV12;
}
