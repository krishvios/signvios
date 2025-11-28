////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiVideoFrame
//
//  File Name:	CstiVideoFrame.h
//
//	Abstract:
//		ACTION - enter description
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTIVIDEOFRAME_H
#define CSTIVIDEOFRAME_H

//
// Includes
//
#include "VideoControl.h"
#include "IstiVideoOutput.h"
#include "CFifo.h"

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//

//
// Globals
//

//
// Class Declaration
//

class CstiVideoFrame
{

public:

	CstiVideoFrame () = default;

	uint32_t unZeroBitsAtStart{0};		// zero padding bits at packet start (sBit)
	uint32_t unZeroBitsAtEnd{0};			// zero padding bits at packet end (eBit)

	IstiVideoPlaybackFrame *m_pPlaybackFrame{nullptr};

	uint32_t m_un32TimeStamp{0};
	bool m_bKeyframe{false};


private:

}; // end CstiVideoFrame Class

#endif // CSTIVIDEOFRAME_H
// end file CstiVideoFrame.h
