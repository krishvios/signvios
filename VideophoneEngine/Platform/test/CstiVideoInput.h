//
//  CstiVideoInput.h
//  gktest
//
//  Created by Dennis Muhlestein on 12/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#ifndef __gktest__CstiVideoInput__
#define __gktest__CstiVideoInput__


#include "CstiSoftwareInput.h"

#include <sys/time.h>

class CstiVideoInput : public CstiSoftwareInput
{

private:
	timeval m_lastInput,m_fpsDelay;

public:

	CstiVideoInput();

	void CaptureEnabledSet (EstiBool bEnabled);
	EstiBool VideoInputRead ( uint8_t *pBytes);
	
	void VideoRecordSizeSet(uint32_t un32Width, uint32_t un32Height);
	void VideoRecordFrameRateSet ( uint32_t un32FrameRate );

};



#endif /* defined(__gktest__CstiVideoInput__) */
