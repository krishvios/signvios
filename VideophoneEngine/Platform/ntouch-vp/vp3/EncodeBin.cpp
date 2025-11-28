#include "EncodeBin.h"
#include "GStreamerElementFactory.h"


GStreamerElement EncodeBin::videoConvertElementGet ()
{
	GStreamerElementFactory vppFactory ("msdkvpp");
	GStreamerElement m_encoderScale = vppFactory.createElement("encode_msdkvpp_convert_element");
		
	m_encoderScale.propertySet ("scaling-mode", 2);
	return m_encoderScale;
}

GStreamerCaps EncodeBin::encodeLegInputCapsGet ()
{
	// This function will set the encode leg input caps 
	// to as large a box as possible inside of 
	// MAX_SELFVIEW_DISPLAY_WIDTHxMAX_SELFVIEW_DISPLAY_HEIGHT
	// or if the cropping is smaller then use the smaller dimensions
	//
	// This will be the size that is sent to the filter just
	// after capture and consequently the display sink
	
	std::stringstream ss;
	
	unsigned width = 0;
	unsigned height = 0;
	unsigned max_width = 0;
	unsigned max_height = 0;
	
	// TODO: No up/down scale
	// Adjust the filter dimensions based on the crop rectangle
	// this does not work right now because the crop rectange is set
	// after this function after a call ends. In order to fix this
	// we should call encoderFilterUpdate at the necessary points
	// in the flow of the UI. For example. at the start and end of
	// calls and after changing zoom settings.
	
	//auto cropRectangle = m_pCaptureBin->cropRectangleGet ();
	
	//stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 1,
	//	vpe::trace ("EncodeBin::encodeLegInputCapsGet: encode width = ", m_width, ", encode height = ", m_height, "\n");
	//	vpe::trace ("EncodeBin::encodeLegInputCapsGet: cropped width = ", cropRectangle.horzZoom, ", cropped height = ", cropRectangle.vertZoom, "\n");
	//);
	
	//max_width = std::min (MAX_SELFVIEW_DISPLAY_WIDTH, cropRectangle.horzZoom);
	//max_height = std::min (MAX_SELFVIEW_DISPLAY_HEIGHT, cropRectangle.vertZoom);

	max_width = MAX_SELFVIEW_DISPLAY_WIDTH;
	max_height = MAX_SELFVIEW_DISPLAY_HEIGHT;

	if ( ((float)max_width / (float)m_width) >= ((float)max_height / (float)m_height))
	{
		width = m_width * ((float)max_height / (float)m_height);
		height = m_height * ((float)max_height / (float)m_height);
	}
	else
	{
		width = m_width * ((float)max_width / (float)m_width);
		height = m_height * ((float)max_width / (float)m_width);
	}
	
	stiDEBUG_TOOL (g_stiVideoEncodeTaskDebug >= 1,
		vpe::trace ("EncodeBin::encodeLegInputCapsGet: width = ", width, ", height = ", height,"\n");
	);
		
	ss << "video/x-raw(memory:DMABuf),"
			<< "format=(string)NV12,"
			<< "width=(int)" << width << ","
			<< "height=(int)" << height << ","
			<< "framerate=(fraction)" << m_framerate << "/1";
			
	return {ss.str()};
}

GStreamerCaps EncodeBin::inputCapsGet ()
{
	// Create Elements
	GStreamerElementFactory elementFactory ("msdkvpp");
	
	if (elementFactory.get () != nullptr)
	{
		std::stringstream ss;

		ss << "video/x-raw(memory:DMABuf),"
			<< "format=(string)NV12,"
			<< "width=(int)" << m_width << ","
			<< "height=(int)" << m_height << ","
			<< "framerate=(fraction)" << m_framerate << "/1";

		return {ss.str()};
	}

	elementFactory = GStreamerElementFactory ("mfxvpp");

	if (elementFactory.get () != nullptr)
	{
		std::stringstream ss;

		ss << "video/x-raw(memory:MFXSurface),"
			<< "format=(string)NV12,"
			<< "width=(int)" << m_width << ","
			<< "height=(int)" << m_height << ","
			<< "framerate=(fraction)" << m_framerate << "/1";

		return {ss.str()};
	}
	
	stiASSERTMSG (false, "EncodeBin::inputCapsGet: Error: no vpp found");

	std::stringstream ss;

	ss << "video/x-raw,"
		<< "format=(string)NV12,"
		<< "width=(int)" << m_width << ","
		<< "height=(int)" << m_height << ","
		<< "framerate=(fraction)" << m_framerate << "/1";

	return {ss.str()};
}

GStreamerCaps EncodeBin::outputCapsGet()
{
	std::string profile{};
	switch (m_codec)
	{
		default:
		case estiVIDEO_H264:
		{
			switch (m_profile)
			{
				default:
					stiASSERTMSG (false, "H264 profile not supported: %d", m_profile);
					[[fallthrough]];
				case estiH264_BASELINE:
				{
					if (m_constraints)
					{
						profile = "constrained-baseline";
					}
					else
					{
						profile = "baseline";
					}
					break;
				}
				case estiH264_MAIN:
				{
					profile = "main";
					break;
				}
				case estiH264_HIGH:
				{
					profile = "high";
					break;
				}
			}

			return GStreamerCaps(mediaTypeGet(),
								 "stream-format", G_TYPE_STRING, "byte-stream",
								 "alignment", G_TYPE_STRING, "au",
								 "profile", G_TYPE_STRING, profile.c_str (),
								 nullptr);
		}

		case estiVIDEO_H265:
		{
			switch (m_profile)
			{
				default:
					stiASSERTMSG (false, "H265 profile not supported: %d", m_profile);
					[[fallthrough]];
				case estiH265_PROFILE_MAIN:
				{
					profile = "main";
					break;
				}
			}

			return GStreamerCaps(mediaTypeGet(),
								 "stream-format", G_TYPE_STRING, "byte-stream",
								 "alignment", G_TYPE_STRING, "au",
								 "profile", G_TYPE_STRING, profile.c_str (),
								 nullptr);
		}
	}

	return {};
}


EstiVideoFrameFormat EncodeBin::bufferFormatGet ()
{
	return estiBYTE_STREAM;
}

const char* EncodeBin::mediaTypeGet()
{
	switch (m_codec)
	{
		default:
		case estiVIDEO_H264:
			return "video/x-h264";

		case estiVIDEO_H265:
			return "video/x-h265";
	}
}


GStreamerElement EncodeBin::encoderGet ()
{
	switch (m_codec)
	{
		default:
		case estiVIDEO_H264:
		{
			GStreamerElement encoder ("msdkh264enc");

			encoder.propertySet("async-depth", 1);

			encoder.propertySet ("gop-size", 0x7fffffff);

			encoder.propertySet ("mbbrc", 0);

			encoder.propertySet("rate-control", 1); // constant bitrate

			//todo: this may need to be adjusted
			encoder.propertySet ("target-usage", 4);

			return encoder;
		}
		case estiVIDEO_H265:
		{
			GStreamerElement encoder ("msdkh265enc");

			encoder.propertySet("async-depth", 1);

			encoder.propertySet ("gop-size", 0x7fffffff);

			encoder.propertySet ("mbbrc", 0);

			encoder.propertySet("rate-control", 1); // constant bitrate

			//todo: this may need to be adjusted
			encoder.propertySet ("target-usage", 7);

			return encoder;
		}
	}

	return {};
}

