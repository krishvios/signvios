#include "EncodeBin.h"
#include "GStreamerElementFactory.h"


GStreamerElement EncodeBin::videoConvertElementGet ()
{
	GStreamerElementFactory imxFactory ("imxvideoconvert_g2d");
	GStreamerElement m_encoderScale = imxFactory.createElement("encode_imx_convert_element");	

	m_encoderScale.propertySet ("videocrop-meta-enable", true);

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
		
	ss << "video/x-raw,"
			<< "format=(string)RGBA,"
			<< "width=(int)" << width << ","
			<< "height=(int)" << height << ","
			<< "framerate=(fraction)" << m_framerate << "/1";
			
	return {ss.str()};
}

GStreamerCaps EncodeBin::inputCapsGet ()
{
	std::stringstream ss;

	ss << "video/x-raw,"
		//<< "format=(string)NV12,"
		<< "width=(int)" << m_width << ","
		<< "height=(int)" << m_height;
	return GStreamerCaps(ss.str());
}

GStreamerCaps EncodeBin::outputCapsGet()
{
	switch (m_codec)
	{
		default:
		case estiVIDEO_H264:
		{
			return GStreamerCaps(mediaTypeGet(),
								 "stream-format", G_TYPE_STRING, "byte-stream",
								 "alignment", G_TYPE_STRING, "au",
								 nullptr);
		}
		case estiVIDEO_H265:
		{
			return GStreamerCaps(mediaTypeGet(),
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
			GStreamerElement encoder ("vpuenc_h264");

			switch (m_profile)
			{
				default:
					stiASSERTMSG (false, "H264 profile not supported: %d", m_profile);
					[[fallthrough]];
				case estiH264_BASELINE:
				{
					encoder.propertySet("profile", 9);
					break;
				}
				case estiH264_MAIN:
				{
					encoder.propertySet("profile", 10);
					break;
				}
				case estiH264_HIGH:
				{
					encoder.propertySet("profile", 11);
					break;
				}
			}

			encoder.propertySet("level", m_level);

			encoder.propertySet("gop-size", 300); // Keyframe every ~10 seconds

			return encoder;
		}
		case estiVIDEO_H265:
		{
			GStreamerElement encoder ("vpuenc_hevc");

			switch (m_profile)
			{
				default:
					stiASSERTMSG (false, "H265 profile not supported: %d", m_profile);
					[[fallthrough]];
				case estiH265_PROFILE_MAIN:
				{
					encoder.propertySet("profile", 0);
					break;
				}
			}

			encoder.propertySet("level", m_level);

			encoder.propertySet("gop-size", 300); // Keyframe every ~10 seconds

			return encoder;
		}
	}

	return {};
}

