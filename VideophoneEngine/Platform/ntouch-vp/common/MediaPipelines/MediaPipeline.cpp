/*!\brief A base class for media pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */

#include "MediaPipeline.h"
#include "stiSVX.h"
#include <cstring>
#include <vector>


void MediaPipeline::registerElementMessageCallback (
	const std::string &messageName,
	std::function<void(const GstStructure &)> callback)
{
	m_elementMessageCallback[messageName] = callback;
}

void MediaPipeline::registerMessageCallback(
	GstMessageType messageType,
	std::function<void(GstMessage &message)> callback)
{
	m_messageCallback[messageType] = callback;
}

gboolean MediaPipeline::busCallback (
	GstBus *pGstBus,
	GstMessage *pGstMessage,
	gpointer pUserData)
{
	auto self = static_cast<MediaPipeline *>(pUserData);

	self->busCallbackHandle (pGstMessage);

	return TRUE;
}

void MediaPipeline::busCallbackHandle (GstMessage *pGstMessage)
{
	static int s_nGraphNumber = 0;
	
	switch (GST_MESSAGE_TYPE(pGstMessage))
	{
		case GST_MESSAGE_ELEMENT:
		{
			const GstStructure *messageStruct = gst_message_get_structure (pGstMessage);

			const gchar *messageName = gst_structure_get_name (messageStruct);

			auto iter = m_elementMessageCallback.find(messageName);

			if (iter != m_elementMessageCallback.end ())
			{
				iter->second(*messageStruct);
			}

			break;
		}

		case GST_MESSAGE_ERROR:
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_error (pGstMessage, &pError, &pDebugInfo);

			std::string errorString ("MediaPipeline::BusCallback: GST_MESSAGE_ERROR\n");

			if (pGstMessage)
			{
				errorString.append (": GST_OBJECT_NAME = ");
				errorString.append (GST_OBJECT_NAME(GST_MESSAGE_SRC (pGstMessage)));
				
				std::string objectName = GST_OBJECT_NAME(GST_MESSAGE_SRC (pGstMessage));
				if (objectName == "capture_camera_element" ||
					objectName.find ("videotestsrc") != std::string::npos)
				{
					vpe::trace ("Error in ", objectName, "\n");
					cameraErrorSignal.Emit();
				}
			}

			if (pError && pError->message)
			{
				errorString.append (": Error message = ");
				errorString.append (pError->message);
			}

			if (pDebugInfo)
			{
				errorString.append (": Debug message = ");
				errorString.append (pDebugInfo);
			}

			errorString.append ("\n");
			stiASSERTMSG (estiFALSE, errorString.c_str ());

			pipelineErrorSignal.Emit();

			stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
				writeGraph ("media_pipeline_error_pipeline");
			);

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

			break;
		}

		case GST_MESSAGE_WARNING:
		{
			GError *pError = nullptr;
			gchar *pDebugInfo = nullptr;

			gst_message_parse_warning (pGstMessage, &pError, &pDebugInfo);

			stiDEBUG_TOOL (g_stiAudioHandlerDebug,

				std::string warningString ("MediaPipeline::BusCallback: GST_MESSAGE_WARNING\n");

				if (pGstMessage)
				{
					warningString.append (": GST_OBJECT_NAME = ");
					warningString.append (GST_OBJECT_NAME(GST_MESSAGE_SRC (pGstMessage)));
				}

				if (pError && pError->message)
				{
					warningString.append (": Warning message = ");
					warningString.append (pError->message);
				}

				if (pDebugInfo)
				{
					warningString.append (": Debug message = ");
					warningString.append (pDebugInfo);
				}

				warningString.append ("\n");
				vpe::trace (warningString);

			);

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

			break;
		}

		case GST_MESSAGE_STATE_CHANGED:
		{
			GstState old_state;
			GstState new_state;
			GstState pending_state;

			gst_message_parse_state_changed (pGstMessage, &old_state, &new_state, &pending_state);

			stiDEBUG_TOOL (g_stiAudioHandlerDebug,
				stiTrace ("MediaPipeline::BusCallback: Element %s changed state from %s to %s. Pending: %s\n",
					GST_OBJECT_NAME (pGstMessage->src),
					gst_element_state_get_name (old_state),
					gst_element_state_get_name (new_state),
					gst_element_state_get_name (pending_state));
			);

			if (!strcmp(GST_OBJECT_NAME(pGstMessage->src), "audio_input_pipeline") && new_state == GST_STATE_PLAYING)
			{
				dspResetSignal.Emit();
			}
			//
			// Dump the graph when the pipeline changes state.
			//

			if (get () && pGstMessage->src == GST_OBJECT(get ()))
			{
				char szFileName[256];

				sprintf (szFileName, "media_pipeline_%s_%s_%s_%d", GST_OBJECT_NAME (pGstMessage->src), gst_element_state_get_name (old_state), gst_element_state_get_name (new_state), s_nGraphNumber);

				s_nGraphNumber++;

				stiDEBUG_TOOL (g_stiDumpPipelineGraphs,
					writeGraph (szFileName);
				);
			}

			break;
		}

		default:
			break;
	}

	auto iter = m_messageCallback.find(GST_MESSAGE_TYPE(pGstMessage));

	if (iter != m_messageCallback.end ())
	{
		iter->second(*pGstMessage);
	}
}
