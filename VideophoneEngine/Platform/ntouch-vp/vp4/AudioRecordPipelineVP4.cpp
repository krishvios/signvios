/*!\brief Specializations for the VP4 audio record pipelines.
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
 */

#include "AudioRecordPipelineVP4.h"


GStreamerElement AudioRecordPipelineVP4::aecElementCreate()
{
	stiHResult hResult {stiRESULT_SUCCESS};

	GStreamerElement retVal;

	retVal = GStreamerElement("webrtcdsp", "audio_input_aec_element");
	stiTESTCOND(retVal.get() != nullptr, stiRESULT_ERROR);

	retVal.propertySet("probe", "audio_output_echo_probe");
	retVal.propertySet("echo-cancel", TRUE);
	retVal.propertySet("echo-suppression-level", 1);
	retVal.propertySet("noise-suppression", TRUE);
	retVal.propertySet("noise-suppression-level", 1);

STI_BAIL:

	if (stiIS_ERROR(hResult))
	{
		retVal.clear();
	}

	return retVal;
}


void AudioRecordPipelineVP4::aecLevelSet (AecSuppressionLevel aecLevel)
{
	m_aecElement.propertySet("echo-suppression-level", aecLevel);
}

