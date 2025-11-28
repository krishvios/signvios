//
//  CstiFFMpegContext.cpp
//  ntouchMac
//
//  Created by Dennis Muhlestein on 8/2/12.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#include "CstiFFMpegContext.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

CstiFFMpegContext* CstiFFMpegContext::m_pShared = NULL;


static void skip_logs ( void *ptr, int level, const char* fmt, va_list vl) {}

CstiFFMpegContext::CstiFFMpegContext() {
	avcodec_register_all();
	av_register_all();
	av_log_set_callback(skip_logs);
}

CstiFFMpegContext* CstiFFMpegContext::Shared() {
	if (!m_pShared) m_pShared = new CstiFFMpegContext;
	
	return m_pShared;
}
