////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiSyncManager
//
//  File Name:	CstiSyncManager.cpp
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:	
//		The CstiSyncManager is to manage the synchronization information for
//		video/audio playback
//
////////////////////////////////////////////////////////////////////////////////


//
// Includes
//
#include "CstiSyncManager.h"
//
// Constants
//

//
// Typedefs
//

//
// Globals
//

//
// Locals
//

//
// Forward Declarations
//

CstiSyncManager::CstiSyncManager ()
{
	stiLOG_ENTRY_NAME (CstiSyncManager::CstiSyncManager);

	Initialize();
}

CstiSyncManager * CstiSyncManager::GetInstance()
{
	 // Meyers singleton: a local static variable (cleaned up automatically at exit)
	static CstiSyncManager oLocalSyncManager;
	return &oLocalSyncManager;
}

void CstiSyncManager::Initialize ()
{
  
	m_un32AudioClockRate = 8000 / stiOSSysClkRateGet ();
	m_un32VideoClockRate = 90000 / stiOSSysClkRateGet ();
	m_un32uSecPerClkRate = m_un32MicroSeconds / stiOSSysClkRateGet ();

	m_un32SyncAudioNTPTimestamp = 0;
	m_un32SyncAudioTimestamp = 0;
	m_un32CurrAudioNTPTimestamp = 0;

	m_un32SyncVideoNTPTimestamp = 0;
	m_un32SyncVideoTimestamp = 0;
	m_un32CurrVideoNTPTimestamp = 0;

	m_bRemoteVideoMute = estiFALSE;
//	m_bRemoteAudioMute = estiTRUE;  // this will be detected and set by AudioPlaybackRead task

	m_nVideoSyncAfterUnMute = 0;
	m_nAudioSyncAfterUnMute = 0;

}

uint32_t CstiSyncManager::AudioTimestampToNTP (uint32_t un32AudioTimestamp, uint32_t un32Duration)
{

	uint32_t un32NTPTime = 0;

	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);

	if(0 == m_un32SyncAudioTimestamp)
	{
#if 1
		return un32NTPTime;
#else
		m_un32SyncAudioTimestamp = un32AudioTimestamp;
#endif    
	}
	
	un32AudioTimestamp += un32Duration;
	
	if(un32AudioTimestamp >= m_un32SyncAudioTimestamp)
	{
		uint32_t un32USec = ((un32AudioTimestamp - m_un32SyncAudioTimestamp) * m_un32uSecPerClkRate) / m_un32AudioClockRate;
		uint32_t un32Sec = 0;
		
		if(un32USec >= m_un32MicroSeconds)
		{
			un32Sec = un32USec / m_un32MicroSeconds;
			un32USec = un32USec % m_un32MicroSeconds;
		}
		
		uint32_t un32fracsec = usec2fracsec(un32USec);
		un32NTPTime = m_un32SyncAudioNTPTimestamp + ntp64to32(un32Sec, un32fracsec);
		
	}
	else
	{
		uint32_t un32USec = ((m_un32SyncAudioTimestamp - un32AudioTimestamp) * m_un32uSecPerClkRate) / m_un32AudioClockRate;
		uint32_t un32Sec = 0;
		
		if(un32USec >= m_un32MicroSeconds)
		{
			un32Sec = un32USec / m_un32MicroSeconds;
			un32USec = un32USec % m_un32MicroSeconds;
		}
		
		uint32_t un32fracsec = usec2fracsec(un32USec);
		un32NTPTime = m_un32SyncAudioNTPTimestamp - ntp64to32(un32Sec, un32fracsec);
	}

	return un32NTPTime;
}

void CstiSyncManager::SetCurrAudioTimestamp (uint32_t un32AudioTimestamp, uint32_t un32Duration)
{


	uint32_t un32NTPTime = AudioTimestampToNTP (un32AudioTimestamp, un32Duration);

	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);

	m_un32CurrAudioNTPTimestamp = un32NTPTime;
}

uint32_t CstiSyncManager::VideoTimestampToNTP (uint32_t un32VideoTimestamp, uint32_t un32Duration)
{
	uint32_t un32NTPTime = 0;

	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);

	if(0 == m_un32SyncVideoTimestamp)
	{
#if 1    
		return un32NTPTime;
#else  
		m_un32SyncVideoTimestamp = un32VideoTimestamp;
#endif    
	}

	un32VideoTimestamp += un32Duration;

	if(un32VideoTimestamp >= m_un32SyncVideoTimestamp)
	{
		uint32_t un32USec = ((un32VideoTimestamp - m_un32SyncVideoTimestamp) * m_un32uSecPerClkRate) / m_un32VideoClockRate;
		uint32_t un32Sec = 0;

		if(un32USec >= m_un32MicroSeconds)
		{
			un32Sec = un32USec / m_un32MicroSeconds;
			un32USec = un32USec % m_un32MicroSeconds;
		}

		uint32_t un32fracsec = usec2fracsec(un32USec);
		un32NTPTime = m_un32SyncVideoNTPTimestamp + ntp64to32(un32Sec, un32fracsec);
	}
	else
	{
		uint32_t un32USec = ((m_un32SyncVideoTimestamp - un32VideoTimestamp) * m_un32uSecPerClkRate) / m_un32VideoClockRate;
		uint32_t un32Sec = 0;
		
		if(un32USec >= m_un32MicroSeconds)
		{
			un32Sec = un32USec / m_un32MicroSeconds;
			un32USec = un32USec % m_un32MicroSeconds;
		}
		
		uint32_t un32fracsec = usec2fracsec(un32USec);
		un32NTPTime = m_un32SyncVideoNTPTimestamp - ntp64to32(un32Sec, un32fracsec);
	}

	return un32NTPTime;

}

void CstiSyncManager::SetCurrVideoTimestamp (uint32_t un32VideoTimestamp, uint32_t un32Duration)
{

	uint32_t un32NTPTime = VideoTimestampToNTP (un32VideoTimestamp, un32Duration);

	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);

	m_un32CurrVideoNTPTimestamp = un32NTPTime;
}
	


void CstiSyncManager::SetAudioSyncInfo(uint32_t un32SyncNtpSec, uint32_t un32SyncNtpFrac, uint32_t un32SyncTimestamp)
{
	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);

	if(estiFALSE == m_bRemoteAudioMute)
	{
		m_nAudioSyncAfterUnMute ++;
	}

	if(m_nAudioSyncAfterUnMute > 0)
	{
		uint32_t un32NTPTimestamp = ntp64to32 (un32SyncNtpSec, un32SyncNtpFrac);

		if (un32NTPTimestamp < m_un32SyncAudioNTPTimestamp)
		{
			// NTP Timestamp has wrapped
			m_nAudioSyncAfterUnMute = 0;
			stiDEBUG_TOOL (g_stiSYNCDebug,
				stiTrace("SYNC:: SetAudioSyncInfo NTPTimestamp has wrapped !!!\n");
			);      
		}

		m_un32SyncAudioNTPTimestamp = un32NTPTimestamp;
		m_un32SyncAudioTimestamp = un32SyncTimestamp;
		
		stiDEBUG_TOOL (g_stiSYNCDebug,
			stiTrace("SYNC:: SetAudioSyncInfo NTPtimestamp = %lu timestamp = %lu\n",
			m_un32SyncAudioNTPTimestamp,
			m_un32SyncAudioTimestamp);
		);
	}
}


void CstiSyncManager::SetVideoSyncInfo(uint32_t un32SyncNtpSec, uint32_t un32SyncNtpFrac, uint32_t un32SyncTimestamp)
{
	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);
	
	if(estiFALSE == m_bRemoteVideoMute)
	{
		m_nVideoSyncAfterUnMute ++;
	}
	
	if(m_nVideoSyncAfterUnMute > 0)
	{		
		uint32_t un32NTPTimestamp = ntp64to32 (un32SyncNtpSec, un32SyncNtpFrac);

		if (un32NTPTimestamp < m_un32SyncVideoNTPTimestamp)
		{
			// NTP Timestamp has wrapped
			m_nVideoSyncAfterUnMute = 0;
			stiDEBUG_TOOL (g_stiSYNCDebug,
				stiTrace("SYNC:: SetVideoSyncInfo NTPTimestamp has wrapped !!!\n");
			);
		}

		m_un32SyncVideoNTPTimestamp = ntp64to32 (un32SyncNtpSec, un32SyncNtpFrac);
		m_un32SyncVideoTimestamp = un32SyncTimestamp;
		
		stiDEBUG_TOOL (g_stiSYNCDebug,
			stiTrace("SYNC:: SetVideoSyncInfo NTPtimestamp = %lu timestamp = %lu\n",
			m_un32SyncVideoNTPTimestamp,
			m_un32SyncVideoTimestamp);
		);
	}
}

EstiBool CstiSyncManager::ValidToSync()
{
	EstiBool bValidToSync = estiFALSE;

	if(estiFALSE == m_bRemoteVideoMute && estiFALSE == m_bRemoteAudioMute)
	{
		if(m_nVideoSyncAfterUnMute > 0 && m_nAudioSyncAfterUnMute > 0)
		{
			bValidToSync = estiTRUE;
		}
	}

	return bValidToSync;

}

void CstiSyncManager::SetRemoteAudioMute(EstiBool bIsMute)
{
	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);

	m_bRemoteAudioMute = bIsMute;

	if(estiFALSE == m_bRemoteAudioMute)
	{
		m_nAudioSyncAfterUnMute = 0;
	}

	stiDEBUG_TOOL (g_stiSYNCDebug,
		if (m_bRemoteAudioMute)
		{
		stiTrace("SYNC:: ***** Remote Audio Mute *****\n");
		}
		else
		{
		stiTrace("SYNC:: ***** Remote Audio UnMute *****\n");
		}
	);
}

void CstiSyncManager::SetRemoteVideoMute(EstiBool bIsMute)
{
	std::lock_guard<std::recursive_mutex> lock (m_LockMutex);

	m_bRemoteVideoMute = bIsMute;

	if(estiFALSE == m_bRemoteVideoMute)
	{
		m_nVideoSyncAfterUnMute = 0;
	}

	stiDEBUG_TOOL (g_stiSYNCDebug,
		if (m_bRemoteVideoMute)
		{
		stiTrace("SYNC:: ***** Remote Video Mute *****\n");
		}
		else
		{
		stiTrace("SYNC:: ***** Remote Video UnMute *****\n");
		}
	);
}

