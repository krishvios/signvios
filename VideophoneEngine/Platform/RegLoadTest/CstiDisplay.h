// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIDISPLAY_H
#define CSTIDISPLAY_H

#include "BaseVideoOutput.h"

#include <sstream>
#include <list>

class CstiVideoPlaybackFrame : public IstiVideoPlaybackFrame
{
public:
	
	CstiVideoPlaybackFrame ()
	{
		m_pBuffer = new uint8_t[720 * 480];
		
		m_un32DataSize = 0;

		m_bFrameIsComplete = false;
		m_bFrameIsKeyframe = false;
	}
	
	virtual ~CstiVideoPlaybackFrame ()
	{
		if (m_pBuffer)
		{
			delete [] m_pBuffer;
			m_pBuffer = NULL;
		}
	}
	
	virtual uint8_t *BufferGet () 	// pointer to the video packet data
	{
		return m_pBuffer;
	}
	
	
	virtual uint32_t BufferSizeGet () const	// size of this buffer in bytes
	{
		return 720 * 480;
	}
	
	
	virtual uint32_t DataSizeGet () const		// Number of bytes in the buffer.
	{
		return m_un32DataSize;
	}
	
	virtual stiHResult BufferSizeSet (uint32_t size)
	{
		m_un32DataSize = size;
		return stiRESULT_SUCCESS;
	}
	
	virtual stiHResult DataSizeSet (
		uint32_t un32DataSize)
	{
		stiHResult hResult = stiRESULT_SUCCESS;
		
		m_un32DataSize = un32DataSize;
		
		return hResult;
	}
	
	virtual bool FrameIsCompleteGet () { return m_bFrameIsComplete; }
	virtual void FrameIsCompleteSet (bool bFrameIsComplete) { m_bFrameIsComplete = bFrameIsComplete; }
	virtual void FrameIsKeyframeSet (bool bFrameIsKeyframe) { m_bFrameIsKeyframe = bFrameIsKeyframe; }
	virtual bool FrameIsKeyframeGet () { return m_bFrameIsKeyframe; }

private:
	
	uint8_t *m_pBuffer;
	uint32_t m_un32DataSize;
	bool m_bFrameIsComplete;
	bool m_bFrameIsKeyframe;
};


class CstiDisplay : public BaseVideoOutput
{
public:

	CstiDisplay ();
	virtual ~CstiDisplay ();
	
	stiHResult Initialize ();

	void Uninitialize ();

	virtual stiHResult DisplayModeCapabilitiesGet (
		uint32_t *pun32CapBitMask);
	
	virtual void DisplaySettingsGet (
		SstiDisplaySettings * pDisplaySettings) const;
	
	virtual stiHResult DisplaySettingsSet (
	   const SstiDisplaySettings * pDisplaySettings);
	
	virtual stiHResult VideoRecordSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const;

	virtual stiHResult RemoteViewPrivacySet (
		EstiBool bPrivacy);
		
	virtual stiHResult RemoteViewHoldSet (
		EstiBool bHold);
		
	virtual stiHResult VideoPlaybackSizeGet (
		uint32_t *pun32Width,
		uint32_t *pun32Height) const;

	virtual stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec);
		
	virtual stiHResult VideoPlaybackStart ();

	virtual stiHResult VideoPlaybackStop ();

	virtual stiHResult VideoPlaybackFrameGet (
		IstiVideoPlaybackFrame **ppVideoFrame);
		
	virtual stiHResult VideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame);

	virtual stiHResult VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame);
		
	virtual EstiH264FrameFormat H264FrameFormatGet ()
	{
		return eH264FrameFormatByteStream;
	}

	virtual int GetDeviceFD () const;

	virtual stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs);

	virtual stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);

	virtual void ScreenshotTake (EstiScreenshotType eType = eScreenshotFull);

	virtual void PreferredDisplaySizeGet(
		unsigned int *displayWidth,
		unsigned int *displayHeight) const {*displayWidth = 0; *displayHeight = 0;};

	// The file descriptor for the signal to be used
	STI_OS_SIGNAL_TYPE m_fdSignal;
	
	EstiVideoCodec m_eVideoCodec;
	
private:

	enum EstiPowerStatus
	{
		ePowerStatusOff = 0,
		ePowerStatusOn,
		ePowerStatusUnknown,
	};
	
	std::list<CstiVideoPlaybackFrame *> m_FrameList;
	std::string m_FileName;
	unsigned int m_unFileCounter;
	FILE *m_pOutputFile;

	bool m_bCECSupported;
	bool m_bTelevisionPowered;
	stiHResult CECStatusCheck();
	EstiPowerStatus CECDisplayPowerGet();
};

#endif // CSTIDISPLAY_H
