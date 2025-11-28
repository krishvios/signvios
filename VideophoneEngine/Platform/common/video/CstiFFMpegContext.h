//
//  CstiFFMpegContext.h
//  ntouchMac
//
//  Created by Dennis Muhlestein on 8/2/12.
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//

#ifndef CstiFFMpegContext_h
#define CstiFFMpegContext_h

/**
 * Calls to avcodec_open/close must be thread safe.  This class helps ensure that
 * although there are different classes (usually an encoder/decoder) that both use
 * ffmpeg, they don't stomp on each other. 
 * 
 * Solves the case where Video playback and record start or stop at the same time
 * and both threads want to open or close the contexts.
 **/

class CstiFFMpegContext {

	public:
		/**
		 * This method is not threadsafe.  Usually
		 * The platform layer initializes classes sequentially so it should be ok.
		 * Once the shared instance has been initialized by one thread, Subsequent
		 * calls to the shared instance are thread safe.
		 **/
		static CstiFFMpegContext* Shared();
		
	private:
		CstiFFMpegContext();
		static CstiFFMpegContext* m_pShared;
};

#endif
