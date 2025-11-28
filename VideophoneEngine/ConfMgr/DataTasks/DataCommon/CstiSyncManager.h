////////////////////////////////////////////////////////////////////////////////
//
// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
//
//  Class Name:	CstiSyncManager
//
//  File Name:	CstiSyncManager.h
//
//  Owner:		Ting-Yu Yang
//
//	Abstract:	
//		See CstiSyncManager.ccp
//
////////////////////////////////////////////////////////////////////////////////
#ifndef CSTISYNCMANAGER_H
#define CSTISYNCMANAGER_H

//
// Includes
//
#include "stiSVX.h"
#include "stiTrace.h"

#include <mutex>
//
// Constants
//
const uint32_t un32MAX_NTP_SEC = (uint32_t) 0x0007fff;

//                     
// Typedefs
//

// 
// Forward Declarations
//

//
// Globals
//

class CstiSyncManager
{

public:

	CstiSyncManager ();
	virtual ~CstiSyncManager () = default;
	CstiSyncManager (const CstiSyncManager& other) = delete;
	CstiSyncManager (CstiSyncManager&& other) = delete;
	CstiSyncManager& operator= (const CstiSyncManager& other) = delete;
	CstiSyncManager& operator= (CstiSyncManager&& other) = delete;

	/*!
	* \brief Retrieves the only reference to the sync manager (singleton obj support)
	*/
	static CstiSyncManager * GetInstance();

	/*!
	* \brief initialze the member variables
	*/
	void Initialize ();

	/*!
	* \brief Set audio clock rate 
	* \param uint32_t un32AudioClockRate - audio clock rate to set
	* \retval None
	*/
	inline void SetAudioClockRate(uint32_t un32AudioClockRate);

	/*!
	* \brief Set video clock rate
	* \param uint32_t un32VideoClockRate - video clock rate to set
	* \retval None
	*/
	inline void SetVideoClockRate(uint32_t un32VideoClockRate);


	/*!
	* \brief calculate and set the corresponding NTP time for current audio timestamp
	* \param uint32_t un32AudioTimestamp - the current audio timestamp
	* \param uint32_t un32Duration - the offset time from the beginning of the current audio timestamp
	* \retval None
	*/
	void SetCurrAudioTimestamp (uint32_t un32AudioTimestamp, uint32_t un32Duration);


	/*!
	* \briefcalculate the corresponding NTP time for current audio timestamp
	* \param uint32_t un32AudioTimestamp - the current  audio timestamp
	* \param uint32_t un32Duration - the offset time from the current audio timestamp
	* \retval None
	*/
	uint32_t AudioTimestampToNTP (uint32_t un32AudioTimestamp, uint32_t un32Duration);

	/*!
	* \brief return the current audio frame's NTP timestamp  
	* \param 
	* \retval uint32_t m_un32CurrAudioNTPTimestamp
	*/
	inline uint32_t GetCurrAudioNTPTimestamp ();

	/*!
	* \briefcalculate and set the corresponding NTP time for current video timestamp
	* \param uint32_t un32AudioTimestamp - the current  video timestamp
	* \param uint32_t un32Duration - the offset time from the current video timestamp
	* \retval None
	*/
	void SetCurrVideoTimestamp (uint32_t un32VideoTimestamp, uint32_t un32Duration);

	/*!
	* \briefcalculate the corresponding NTP time for current video timestamp
	* \param uint32_t un32AudioTimestamp - the current  video timestamp
	* \param uint32_t un32Duration - the offset time from the current video timestamp
	* \retval None
	*/
	uint32_t VideoTimestampToNTP (uint32_t un32VideoTimestamp, uint32_t un32Duration);

	/*!
	* \brief return the current audio frame's NTP timestamp  
	* \param 
	* \retval uint32_t m_un32CurrVideoNTPTimestamp
	*/
	inline uint32_t GetCurrVideoNTPTimestamp ();

	/*!
	* \brief Set the synchronization information (NTP time, timestamp) for audio
	* \param uint32_t un32SyncNtpSec - the second part of 64-bit ntp timestamp 
	* \param uint32_t un32SyncNtpSec - the fraction part of 64-bit ntp timestamp 
	* \param uint32_t un32SyncTimestamp - the audio timestamp
	* \retval uint32_t NTP time in 32-bit fixed point format
	*/
	void SetAudioSyncInfo(uint32_t un32SyncNtpSec, uint32_t un32SyncNtpFrac, uint32_t un32SyncTimestamp);

	/*!
	* \brief Set the synchronization information (NTP time, timestamp) for video
	* \param uint32_t un32SyncNtpSec - the second part of 64-bit ntp timestamp 
	* \param uint32_t un32SyncNtpSec - the fraction part of 64-bit ntp timestamp 
	* \param uint32_t un32SyncTimestamp - the video timestamp
	* \retval uint32_t NTP time in 32-bit fixed point format
	*/
	void SetVideoSyncInfo(uint32_t un32SyncNtpSec, uint32_t un32SyncNtpFrac, uint32_t un32SyncTimestamp);


	/*!
	* \brief Set the remote audio mute mode
	* \param EstiBool bRemoteAudioMute - EstiTRUE for mute, otherwise unmute
	*/
	void SetRemoteAudioMute(EstiBool bRemoteAudioMute);

	
	/*!
	* \brief Set the remote video mute mode
	* \param EstiBool bRemoteAudioMute - EstiTRUE for mute, otherwise unmute
	*/
	void SetRemoteVideoMute(EstiBool bRemoteAudioMute);

	
	/*!
	* \brief Get the current mute mode for remote audio 
	* \retval EstiBool EstiTRUE for mute, otherwise unmute
	*/
	inline EstiBool GetRemoteAudioMute();
#if 0
	/*!
	* \brief Get the current mute mode for remote video . 
	* \retval EstiBool EstiTRUE for mute, otherwise unmute
	*/
	inline EstiBool GetRemoteVideoMute(void);

#endif


	/*!
	* \brief Get to know if the current timing information is enough to perform lip sync 
	* \retval EstiBool EstiTRUE for OK to sync, otherwise not OK
	*/
	EstiBool ValidToSync();

	/*!
	* \brief reset to get new sync information for video
	*/
	inline void VideoSyncReset();

	/*!
	* \brief reset to get new sync information for audio
	*/
	inline void AudioSyncReset();

private:
	
	/*!
	* \brief convert NTP time (UTC time relative to Jan 1, 1970) from 64-bit fixed point to 32-bit fixed point format. 
	* \param uint32_t ntp_sec - second part of 64-bit ntp timestamp 
	* \param uint32_t ntp_frac - fraction part of 64-bit ntp timestamp 
	* \retval uint32_t NTP time in 32-bit fixed point format
	*/
	static inline uint32_t ntp64to32(uint32_t ntp_sec, uint32_t ntp_frac);

	/*!
	* \brief convert microseconds to fraction of second in NTP 32-bit fixed format format. 
	* \param uint32_t usec - microseconds
	* \retval uint32_t NTP time in 32-bit fixed point
	*/
	static inline uint32_t usec2fracsec(uint32_t usec);

	uint32_t m_un32MicroSeconds{1000000};
	uint32_t m_un32SyncAudioNTPTimestamp{0};
	uint32_t m_un32SyncAudioTimestamp{0};
	uint32_t m_un32CurrAudioNTPTimestamp{0};

	uint32_t m_un32SyncVideoNTPTimestamp{0};
	uint32_t m_un32SyncVideoTimestamp{0};
	uint32_t m_un32CurrVideoNTPTimestamp{0};

	uint32_t m_un32AudioClockRate{0};
	uint32_t m_un32VideoClockRate{0};

	uint32_t m_un32uSecPerClkRate{0};

	EstiBool m_bRemoteAudioMute{estiFALSE};
	EstiBool m_bRemoteVideoMute{estiFALSE};

	int m_nAudioSyncAfterUnMute{0};
	int m_nVideoSyncAfterUnMute{0};

	std::recursive_mutex m_LockMutex;

};

uint32_t CstiSyncManager::GetCurrVideoNTPTimestamp ()
{
	return m_un32CurrVideoNTPTimestamp;
}

uint32_t CstiSyncManager::GetCurrAudioNTPTimestamp ()
{
	return m_un32CurrAudioNTPTimestamp;
}

void CstiSyncManager::SetAudioClockRate(uint32_t un32AudioClockRate)
{
	m_un32AudioClockRate = un32AudioClockRate / stiOSSysClkRateGet ();
}

void CstiSyncManager::SetVideoClockRate(uint32_t un32VideoClockRate)
{
	m_un32VideoClockRate = un32VideoClockRate / stiOSSysClkRateGet ();
}

EstiBool CstiSyncManager::GetRemoteAudioMute()
{
	return m_bRemoteAudioMute;
}

#if 0
EstiBool CstiSyncManager::GetRemoteVideoMute(void)
{
	return m_bRemoteVideoMute;
}
#endif


uint32_t CstiSyncManager::ntp64to32(uint32_t ntp_sec, uint32_t ntp_frac)
{
	return (uint32_t) ((((ntp_sec) & un32MAX_NTP_SEC) << 16) | (((ntp_frac) & 0xffff0000) >> 16));
}

//
// convert microseconds to fraction of second * 2^32 (i.e., the lsw of
// a 64-bit ntp timestamp).  This routine uses the factorization
// 2^32/10^6 = 4096 + 256 - 1825/32 which results in a max conversion
// error of 3 * 10^-7 and an average error of half that.
//
uint32_t CstiSyncManager::usec2fracsec(uint32_t usec)
{
	uint32_t t = (usec * 1825) >> 5;
	return ((usec << 12) + (usec << 8) - t);
}

void CstiSyncManager::VideoSyncReset ()
{
	m_nVideoSyncAfterUnMute = 0;
}

void CstiSyncManager::AudioSyncReset ()
{
	m_nAudioSyncAfterUnMute = 0;
}




						
#endif //#ifndef CSTISYNCMANAGER_H
