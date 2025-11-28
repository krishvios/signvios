/*!
 * \file CstiVideoOutput.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2015-2021 Sorenson Communications, Inc. -- All rights reserved
 */
#pragma once

#include "stiSVX.h"
#include "stiError.h"

#include "CstiVideoOutputBase2.h"

#include "CstiMonitorTask.h"
#include "CstiVideoInput.h"
#include "GStreamerPipeline.h"
#include "CstiCECControl.h"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/gstbufferlist.h>

#include <iostream>
#include <memory>
#include <queue>
#include <condition_variable>
#include <mutex>

//
// Constants
//

#include "CstiVideoPlaybackFrame.h"

#define ENABLE_VIDEO_OUTPUT


class CstiVideoOutput: public CstiVideoOutputBase2
{
public:

	/*!
	 * \brief Get instance
	 *
	 * \return IstiVideoOutput*
	 */
	CstiVideoOutput ();

	~CstiVideoOutput () override = default;

	stiHResult Initialize (
		CstiMonitorTask *pMonitorTask,
		CstiVideoInput *pVideoInput,
		std::shared_ptr<CstiCECControl> cecControl);

	std::string decodeElementName(EstiVideoCodec eCodec) override;

private:
	static constexpr int NUM_DEC_PRIME_BITSTREAM_BUFS = 3;
	static constexpr const int VIDEO_FILE_PAN_SHIFT_TIMEOUT = 15000; //15 seconds
	static constexpr const int VIDEO_FILE_PAN_PORTAL_WIDTH = 1536;
	static constexpr const int VIDEO_FILE_PAN_PORTAL_HEIGHT = 864;
	static constexpr const int VIDEO_FILE_PAN_WIDTH = 1920;
	static constexpr const int VIDEO_FILE_PAN_HEIGHT = 1080;

	CstiMonitorTask *m_pMonitorTask = nullptr;

};
