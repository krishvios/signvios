#include "FrameCaptureBin.h"
#include "GStreamerElementFactory.h"


GStreamerElement FrameCaptureBin::jpegEncoderGet()
{
	// Create the jpeg encoder.
	GStreamerElement jpegEncoder = GStreamerElement("msdkmjpegenc", "msdk_jpeg_encoder_element");

	return jpegEncoder;
}
