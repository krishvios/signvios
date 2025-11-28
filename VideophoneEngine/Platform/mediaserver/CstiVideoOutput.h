#pragma once

#include "stiOSSignal.h"
#include "BaseVideoOutput.h"
#include "IPlatformVideoOutput.h"
#include "CstiVideoCallback.h"
#include "CstiConvertToH264.h"
#include "MSPacketContainer.h"
#include <sstream>
#include <vector>
#include <list>
#include <unordered_map>

class CstiVideoPlaybackFrame : public IstiVideoPlaybackFrame
{
public:
	

	CstiVideoPlaybackFrame ()
	{
		
		m_pBuffer = new uint8_t[m_un32BufferSize];
		
		m_un32DataSize = 0;
		m_bFrameIsComplete = false;
		m_bFrameIsKeyframe = false;
	}
	
	virtual ~CstiVideoPlaybackFrame ()
	{
	}
	
	virtual uint8_t *BufferGet ()		// pointer to the video packet data
	{
		return m_pBuffer;
	}
	
	
	virtual uint32_t BufferSizeGet () const	// size of this buffer in bytes
	{
		return m_un32BufferSize;
	}


	virtual stiHResult BufferSizeSet(uint32_t un32BufferSize)	// size of this buffer in bytes to set
	{
		if (m_un32BufferSize < un32BufferSize)
		{
			m_un32BufferSize = un32BufferSize;
			uint8_t* pBuffer = new uint8_t[m_un32BufferSize];
			memcpy(pBuffer, m_pBuffer, m_un32DataSize);
			delete m_pBuffer;
			m_pBuffer = pBuffer;
			
		}
		return stiRESULT_SUCCESS;
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
		
		return hResult;
	}

	virtual bool FrameIsCompleteGet () { return m_bFrameIsComplete; }
	virtual void FrameIsCompleteSet (bool bFrameIsComplete) { m_bFrameIsComplete = bFrameIsComplete; }
	virtual void FrameIsKeyframeSet (bool bFrameIsKeyframe) { m_bFrameIsKeyframe = bFrameIsKeyframe; }
	virtual bool FrameIsKeyframeGet () { return m_bFrameIsKeyframe; }

private:
	uint32_t m_un32BufferSize = 2000;
	uint8_t *m_pBuffer;
	uint32_t m_un32DataSize;
	bool m_bFrameIsComplete;
	bool m_bFrameIsKeyframe;
};


class CstiVideoOutput : public BaseVideoOutput, public IPlatformVideoOutput
{
public:
	CstiVideoOutput ();

	CstiVideoOutput (uint32_t nCallIndex);

	~CstiVideoOutput () override;

	stiHResult Initialize ();
	
	void Uninitialize ();

	void DisplaySettingsGet (SstiDisplaySettings * pDisplaySettings) const override;

	stiHResult DisplaySettingsSet (const SstiDisplaySettings * pDisplaySettings) override;

	stiHResult RemoteViewPrivacySet (EstiBool bPrivacy) override;

	stiHResult RemoteViewHoldSet (EstiBool bHold) override;

	stiHResult VideoPlaybackSizeGet (uint32_t *punWidth, uint32_t *punHeight) const override;

	stiHResult VideoPlaybackCodecSet (EstiVideoCodec eVideoCodec) override;

	void RequestKeyFrame () override;

	EstiVideoCodec VideoPlaybackCodecGet () override;

	stiHResult VideoPlaybackStart () override;

	stiHResult VideoPlaybackStop () override;

	stiHResult VideoPlaybackFrameGet (IstiVideoPlaybackFrame **ppVideoFrame) override;

	stiHResult VideoPlaybackFramePut (IstiVideoPlaybackFrame *pVideoFrame) override;

	stiHResult VideoPlaybackFrameDiscard (IstiVideoPlaybackFrame *pVideoFrame) override;

	int GetDeviceFD () const override;

	stiHResult VideoCodecsGet (std::list<EstiVideoCodec> *pCodecs) override;

	stiHResult CodecCapabilitiesGet (EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps) override;

	stiHResult CodecCapabilitiesSet (EstiVideoCodec eCodec, SstiVideoCapabilities *pstCaps) override;

	void ScreenshotTake (EstiScreenshotType eType = eScreenshotFull) override;

	void BeginRecordingFrames () override;

	void PreferredDisplaySizeGet (unsigned int *displayWidth, unsigned int *displayHeight) const override;

	void instanceRelease () override;

	bool hasKeyFrameGet () override;

	// The file descriptor for the signal to be used
	STI_OS_SIGNAL_TYPE m_signal = nullptr;

	static std::unordered_map<uint32_t, std::shared_ptr<CstiVideoOutput>> m_videoOutputs;
	
private:
	
	MSPacketContainer m_videoContainer;
	CstiVideoPlaybackFrame m_videoParsingFrame;
	bool m_isSignMailRecording = false;
	bool m_isRecording = false;
	bool m_hasKeyFrame = false;

	stiMUTEX_ID m_videoLockMutex;

	void bufferCleanup(uint8_t *pBuffer, uint32_t un32BufferLength);
	bool writePacket(std::shared_ptr<SstiMSPacket> packet);
	void deleter (SstiMSPacket *packet);
};
