// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2016 Sorenson Communications, Inc. -- All rights reserved

#include "CstiVideoOutput.h"
#include "GStreamerBuffer.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory>

#include "stiRemoteLogEvent.h"
#include "stiTools.h"
#include "stiTrace.h"
#include "VideoControl.h"

#define ENABLE_REMOTE_VIEW_SETTINGS_SET
#define ENABLE_SELF_VIEW_SETTINGS_SET

#define DECODE_LATENCY_IN_NS      (70 * GST_MSECOND)


/*!
 * \brief default constructor
 */
CstiVideoOutput::CstiVideoOutput ()
:
	CstiVideoOutputBase2 (MAX_DISPLAY_WIDTH * MAX_DISPLAY_HEIGHT, NUM_DEC_BITSTREAM_BUFS)
{

}

/*!
 * \brief Initialize
 * Only called once for object
 *
 * \return stiHResult
 */
stiHResult CstiVideoOutput::Initialize (
	CstiMonitorTask *pMonitorTask,
	CstiVideoInput *pVideoInput,
	std::shared_ptr<CstiCECControl> cecControl)
{
	stiHResult hResult = stiRESULT_SUCCESS;

	stiTESTCOND (pMonitorTask, stiRESULT_ERROR);
	stiTESTCOND (pVideoInput, stiRESULT_ERROR);
	
#ifdef CEC_ENABLED
	stiTESTCOND (cecControl != nullptr, stiRESULT_ERROR);
	m_cecControl = cecControl;
#endif

	m_pMonitorTask = pMonitorTask;

	m_pVideoInput = pVideoInput;

	if (nullptr == m_Signal)
	{
		EstiResult eResult = stiOSSignalCreate (&m_Signal);
		stiTESTCOND (estiOK == eResult, stiRESULT_TASK_INIT_FAILED);
		stiOSSignalSet2 (m_Signal, 0);
	}

	stiOSStaticDataFolderGet (&m_destDir);

	if (m_destDir.empty())
	{
		vpe::trace ("Can't Get static data folder\n");
		stiTHROW (stiRESULT_ERROR);
	}

#ifdef ENABLE_FRAME_CAPTURE
	hResult = FrameCapturePipelineStart ();
	stiTESTRESULT ();
#endif

STI_BAIL:
	return (hResult);
}

std::string CstiVideoOutput::decodeElementName(EstiVideoCodec eCodec) 
{
	return "vpudec";
}
