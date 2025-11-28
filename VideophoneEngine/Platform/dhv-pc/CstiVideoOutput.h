#pragma once

#include <queue>

#include "IPlatformVideoOutput.h"
#include "BaseVideoOutput.h"
#include "IVideoDecoder.h"
#include "IVideoDisplayFrame.h"
#include "CstiEventQueue.h"
#include "stiOSMutex.h"
#include <time.h>

class CstiVideoOutput : public CstiEventQueue, public BaseVideoOutput, public IPlatformVideoOutput
{
public:
	CstiVideoOutput();
	~CstiVideoOutput ();
	virtual stiHResult CodecCapabilitiesGet (EstiVideoCodec codec, SstiVideoCapabilities *videoCapabilities) override;
	virtual void DisplaySettingsGet (SstiDisplaySettings *displaySettings) const override;
	virtual stiHResult DisplaySettingsSet (const SstiDisplaySettings *displaySettings) override;
	virtual int GetDeviceFD () const override;
	virtual void PreferredDisplaySizeGet (unsigned int *displayWidth, unsigned int *displayHeight) const override;
	virtual stiHResult RemoteViewHoldSet (EstiBool hold) override;
	virtual stiHResult RemoteViewPrivacySet (EstiBool privacy) override;
	virtual stiHResult VideoCodecsGet (std::list<EstiVideoCodec> *codecs) override;
	virtual stiHResult VideoPlaybackCodecSet (EstiVideoCodec videoCodec) override;
	virtual stiHResult VideoPlaybackFrameDiscard (IstiVideoPlaybackFrame *videoFrame) override;
	virtual stiHResult VideoPlaybackFrameGet (IstiVideoPlaybackFrame **ppVideoFrame) override;
	virtual stiHResult VideoPlaybackFramePut (IstiVideoPlaybackFrame *videoFrame) override;
	virtual stiHResult VideoPlaybackSizeGet (uint32_t *pWidth, uint32_t *pHeight) const override;
	virtual stiHResult VideoPlaybackStart () override;
	virtual stiHResult VideoPlaybackStop () override;

	virtual stiHResult CodecCapabilitiesSet (EstiVideoCodec codec, SstiVideoCapabilities *videoCapabilities) override;

	void testCodecSet (EstiVideoCodec testCodec);
	void returnFrame (IVideoDisplayFrame* frame);
	
private:
	bool IsHevcAllowed ();
	void displayBlackFrame ();

	const char* ENABLE_H265 = "EnableH265";

	EstiVideoCodec m_codec = estiVIDEO_NONE;
	std::map<EstiVideoCodec, SstiVideoCapabilities*> m_codecCapabilities;
	stiMUTEX_ID m_framesMutex;
	stiMUTEX_ID m_decoderMutex;
	size_t m_framesCreated = 0;
	// This queue holds frames that are used for decoding
	std::queue<IstiVideoPlaybackFrame *> m_emptyFrames;
	// This queue holds frames that will be sent to the UI
	std::queue<IVideoDisplayFrame*> m_displayFrames;

	IVideoDecoder *m_decoder = nullptr;
	timeval m_lastIFrame;
	int m_width = 0;
	int m_height = 0;
	uint64_t m_decodedFrameCount = 0;
	uint64_t m_discaredDisplayFrameCount = 0;
	EstiVideoCodec m_testCodec = estiVIDEO_NONE;
	STI_OS_SIGNAL_TYPE m_fdSignal = nullptr;
	bool m_isDecoding = false;
};

