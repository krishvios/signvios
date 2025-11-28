 // Sorenson Communications Inc. Confidential. --  Do not distribute
 // Copyright 2020 Sorenson Communications, Inc. -- All rights reserved

#include "DecodePipeline.h"

#define H264_DECODER "264dec"
#define H265_DECODER "265dec"

#define DECODE_ERROR "[Corruption]"
#define MS_BETWEEN_KEYFRAME_REQUESTS 333


void DecodePipeline::registerBusCallbacks ()
{
	registerMessageCallback(GST_MESSAGE_WARNING,
		[this](GstMessage &gstMessage)
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_warning (&gstMessage, &pError, &pDebugInfo);

			std::string elementName {GST_OBJECT_NAME(GST_MESSAGE_SRC (&gstMessage))};
			if (elementName.compare (0, strlen (H264_DECODER), H264_DECODER) == 0 ||
				elementName.compare (0, strlen (H265_DECODER), H265_DECODER) == 0)
			{
				if (pError && pError->message)
				{
					std::string message {pError->message};
					auto now = std::chrono::steady_clock::now ();

					if (message.compare (0, strlen (DECODE_ERROR), DECODE_ERROR) == 0 &&
						now > m_lastKeyframeRequestTime + std::chrono::milliseconds {MS_BETWEEN_KEYFRAME_REQUESTS})
					{
						decodeErrorSignal.Emit ();
						m_lastKeyframeRequestTime = now;
					}
				}
			}

			if (pError)
			{
				g_error_free (pError);
				pError = nullptr;
			}

			if (pDebugInfo)
			{
				g_free (pDebugInfo);
				pDebugInfo = nullptr;
			}
		});

	registerMessageCallback(GST_MESSAGE_ERROR, 
		[this](GstMessage &gstMessage)
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_error (&gstMessage, &pError, &pDebugInfo);

			if (pError &&
				pError->domain == GST_STREAM_ERROR &&
				pError->code == GST_STREAM_ERROR_FAILED)
			{
				decodeStreamingErrorSignal.Emit();
			}
			
			if (pError)
			{
				g_error_free (pError);
				pError = nullptr;
			}

			if (pDebugInfo)
			{
				g_free (pDebugInfo);
				pDebugInfo = nullptr;
			}
		});
}
