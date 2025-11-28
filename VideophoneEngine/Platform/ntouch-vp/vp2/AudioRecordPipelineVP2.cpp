/*!\brief Specializations for the VP2 audio record pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2025 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioRecordPipelineVP2.h"


void AudioRecordPipelineVP2::micLevelProcess (
	const GstStructure &messageStruct)
{
	gint channels;
	const GValue *list;

	list = gst_structure_get_value(&messageStruct, "rms");

	channels = gst_value_list_get_size(list);

	for (int i = 0; i < channels; i++)
	{
		const GValue *value = gst_value_list_get_value (list, i);

		gdouble rms_DB = g_value_get_double (value);

		stiDEBUG_TOOL(g_stiAudioEncodeDebug > 1,
			stiTrace ("CstiAudioInputBase::micLevelProcess channel: %d, rms_DB: %f\n", i, rms_DB);
		);

		const int volFactor = 20; // Normalize the level for the audio meter
		int audioLevel = pow(10, (rms_DB + volFactor) / 20) * 100;

		m_audioLevelSignal.Emit (audioLevel);
	}
}
