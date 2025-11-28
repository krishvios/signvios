//
//  CstiVideoInput.cpp
//  gktest
//
//  Created by Dennis Muhlestein on 12/5/12.
//  Copyright (c) 2012 Sorenson Communications. All rights reserved.
//

#include "CstiVideoInput.h"

CstiVideoInput::CstiVideoInput()
{
	gettimeofday(&m_lastInput, NULL);
	m_fpsDelay.tv_sec=0;
	m_fpsDelay.tv_usec=66666; // 15 fps
}

void CstiVideoInput::CaptureEnabledSet (EstiBool bEnabled)
{
}

EstiBool CstiVideoInput::VideoInputRead ( uint8_t *pBytes)
{
  // Our encoder just plays pre-recorded frames so we just delay
	timeval now, delta;
	timeradd(&m_lastInput, &m_fpsDelay, &delta);
	gettimeofday(&now,NULL);
	if (timercmp(&delta,&now,-) > 0 )
	{
		// usleep the delta amount
		timeval diff;
		timersub(&delta,&now,&diff);
		int slv=diff.tv_usec;
		usleep( slv );
	} 
	gettimeofday(&m_lastInput,NULL);
	return estiTRUE;
}

void CstiVideoInput::VideoRecordSizeSet(uint32_t un32Width, uint32_t un32Height)
{
  // we'll just always support CIF
}

void CstiVideoInput::VideoRecordFrameRateSet ( uint32_t un32FrameRate )
{
 // assumes more than 1 fps ;)
	m_fpsDelay.tv_usec = 1e6 / un32FrameRate;
}
