/*!
* \file CstiSoftwareDisplay.h
* \brief Platform layer Software VideoOutput Abstract Class
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2015 Sorenson Communications, Inc. -- All rights reserved
*/

#ifndef CSTISOFTWAREDISPLAY_H
#define CSTISOFTWAREDISPLAY_H

#ifndef WIN32
#include <sys/time.h>
#endif

#include <queue>


#include "BaseVideoOutput.h"
#include "IstiVideoDecoder.h"
#include "IstiVideoOutput.h"

#include "CstiFFMpegDecoder.h"
#include "VideoFrame.h"

#if APPLICATION == APP_NTOUCH_MAC || APPLICATION == APP_NTOUCH_IOS
#include "CstiVTDecoder.h"
#endif
#include "CstiOsTaskMQ.h"
#include "stiEventMap.h"
#include "CstiEvent.h"
#include "stiOSSignal.h"
#include <map>


const int MIN_IFRAME_REQUEST_DELTA=2;
const int MAX_IFRAME_REQUEST_DELTA=30;
const int DEFAULT_PLAYBACK_FRAME_SIZE=1024*7;// start with 7k buffers

class CstiVideoPlaybackFrame : public IstiVideoPlaybackFrame
{
public:
	
	CstiVideoPlaybackFrame ()
	{
		++m_un32BufferCount;
		m_pBuffer = (uint8_t*)malloc ( DEFAULT_PLAYBACK_FRAME_SIZE );
		m_un32DataSize = 0; //DEFAULT_PLAYBACK_FRAME_SIZE;
		m_un32BufferSize = DEFAULT_PLAYBACK_FRAME_SIZE;
		m_bFrameIsComplete = false;
		m_bFrameIsKeyframe = false;
	}
	
	~CstiVideoPlaybackFrame ()
	{
		if (m_pBuffer)
		{
			free(m_pBuffer);
			m_pBuffer = NULL;
		}
		--m_un32BufferCount;
	}
	
	
	virtual uint8_t *BufferGet ()	// pointer to the video packet data
	{
		return m_pBuffer;
	}
	
	
	virtual uint32_t BufferSizeGet () const	// size of this buffer in bytes
	{
		return m_un32BufferSize;
	}
	
	virtual stiHResult BufferSizeSet (uint32_t un32BufferSize)
	{
		stiHResult hResult = stiRESULT_SUCCESS;

		uint8_t *pNewBuffer = (uint8_t*)realloc( m_pBuffer, un32BufferSize );
		
		stiTESTCOND_NOLOG (pNewBuffer, stiRESULT_MEMORY_ALLOC_ERROR);

		m_pBuffer = pNewBuffer;
		m_un32BufferSize=un32BufferSize;

STI_BAIL:

		return hResult;
	}
	
	virtual uint32_t DataSizeGet () const		// Number of bytes in the buffer.
	{
		return m_un32DataSize;
	}
	
	
	virtual stiHResult DataSizeSet (
									uint32_t un32DataSize)
	{
		stiHResult hResult = stiRESULT_SUCCESS;		
		m_un32DataSize = un32DataSize;
		stiASSERT(m_un32DataSize <= m_un32BufferSize);
		return hResult;
	}
	
	virtual bool FrameIsCompleteGet () { return m_bFrameIsComplete; }
	virtual void FrameIsCompleteSet (bool bFrameIsComplete) { m_bFrameIsComplete = bFrameIsComplete; }
	virtual void FrameIsKeyframeSet (bool bFrameIsKeyframe) { m_bFrameIsKeyframe = bFrameIsKeyframe; }
	virtual bool FrameIsKeyframeGet () { return m_bFrameIsKeyframe; }

	static uint32_t CreatedGet() {
		return m_un32BufferCount;
	}
	
private:
	
	uint8_t *m_pBuffer;
	uint32_t m_un32BufferSize;
	uint32_t m_un32DataSize;
	static uint32_t m_un32BufferCount;
	bool m_bFrameIsComplete;
	bool m_bFrameIsKeyframe;
};

/**
 * 
 **/
class CstiSoftwareDisplay : public CstiOsTaskMQ, public BaseVideoOutput
{
public:

	enum EEventType 
	{
		estiVP_DELIVER_FRAME = 512,
	};

	CstiSoftwareDisplay ();
	~CstiSoftwareDisplay();
	
	virtual stiHResult Initialize ();
	
	virtual void Uninitialize ();

	virtual void DisplaySettingsGet (
		IstiVideoOutput::SstiDisplaySettings * pDisplaySettings) const;
	
	virtual stiHResult DisplaySettingsSet (
		const IstiVideoOutput::SstiDisplaySettings * pDisplaySettings);
	
	virtual stiHResult RemoteViewPrivacySet (
		EstiBool bPrivacy);
		
	virtual stiHResult RemoteViewHoldSet (
		EstiBool bHold);
		
	virtual stiHResult VideoPlaybackSizeGet (
		uint32_t *punWidth,
		uint32_t *punHeight) const;

	virtual stiHResult VideoPlaybackCodecSet (
		EstiVideoCodec eVideoCodec);
		
	virtual stiHResult VideoPlaybackStart ();

	virtual stiHResult VideoPlaybackStop ();

	virtual stiHResult VideoPlaybackFramePut (
		vpe::VideoFrame * pVideoFrame) { /* unused */
		return stiRESULT_SUCCESS; }
		
	virtual stiHResult VideoPlaybackFrameGet (
		IstiVideoPlaybackFrame **ppVideoFrame);
		
	virtual stiHResult VideoPlaybackFramePut (
		IstiVideoPlaybackFrame *pVideoFrame);

	virtual stiHResult VideoPlaybackFrameDiscard (
		IstiVideoPlaybackFrame *pVideoFrame);
		
	virtual int GetDeviceFD () const;

	virtual stiHResult VideoCodecsGet (
		std::list<EstiVideoCodec> *pCodecs);

	virtual stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);

	virtual stiHResult CodecCapabilitiesSet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);	


	virtual void PreferredDisplaySizeGet (
		unsigned int *displayWidth,
		unsigned int *displayHeight) const;


	virtual stiHResult ConvertNalUnitsSet(
		bool convertNalUnits);

	virtual stiHResult SwapSizeBytesSet(
		bool swapSizeBytes);

#if DEVICE == DEV_X86
	virtual void ScreenshotTake (EstiScreenshotType eType = eScreenshotFull);
#endif
	
	// The file descriptor for the signal to be used
	STI_OS_SIGNAL_TYPE m_fdSignal;
	
	// NOTE why public on this?
	EstiVideoCodec m_eVideoCodec;
	
	/*
	 * Implementations should copy bytes and queue so this function returns asap.
	 */
	virtual void DisplayYUVImage ( uint8_t *pBytes, uint32_t nWidth, uint32_t nHeight )=0;
	
#if APPLICATION == APP_NTOUCH_MAC
	virtual void DisplayCMSampleBuffer ( uint8_t *pBytes, uint32_t nWidth, uint32_t nHeight )=0;
#endif

#if APPLICATION == APP_NTOUCH_ANDROID || APPLICATION == APP_DHV_ANDROID
	/*!
	 * \brief Notify orientation change.
	 */
	virtual void OrientationChanged () = 0;
#endif
	
protected:
	IstiVideoDecoder *m_pVideoDecoder;

#if APPLICATION == APP_NTOUCH_MAC
	CstiVTDecoder *m_pVTDecoder;
#endif
	
	stiMUTEX_ID m_DecoderMutex;
	std::queue<IstiVideoPlaybackFrame*> m_availableFrames;
	
	timeval m_lastIFrame;
	bool m_decoding;
	bool m_convertNalUnits;
	bool m_swapSizeBytes;
	uint32_t m_width;
	uint32_t m_height;

	
	int Task () override;
	
	void FreeFrames(int); // cleanup memory
    
    virtual stiHResult InitDecoder();
	virtual IstiVideoPlaybackFrame *AllocateVideoPlaybackFrame();

	stiDECLARE_EVENT_MAP (CstiSoftwareDisplay);
	stiDECLARE_EVENT_DO_NOW (CstiSoftwareDisplay);

	//
	// Event Handlers
	//
	EstiResult stiEVENTCALL EventDeliverFrame (
		IN void* poEvent);

	private:
		std::map<EstiVideoCodec, SstiVideoCapabilities*> CodecCapabilitiesLookup;
};

#endif // CSTIDISPLAY_H
