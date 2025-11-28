#include "FrameCaptureBin.h"
#include "GStreamerElementFactory.h"

FrameCaptureBin::FrameCaptureBin ()
:
	m_cropScaleBin("framecap_ptz", true, true)
{
}
