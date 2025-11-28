/*!
 * \file AppleVideoOutput.h
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "BaseVideoOutput.h"
#include "CstiTimer.h"
#include <CoreMedia/CoreMedia.h>
#include <VideoToolbox/VideoToolbox.h>
#include <queue>
#include <mutex>

namespace vpe
{

class AppleVideoOutput : public BaseVideoOutput
{
public:
	AppleVideoOutput ();
	virtual ~AppleVideoOutput ();

	AppleVideoOutput (const AppleVideoOutput &) = delete;
	AppleVideoOutput (AppleVideoOutput &&) = delete;
	AppleVideoOutput &operator= (const AppleVideoOutput &) = delete;
	AppleVideoOutput &operator= (AppleVideoOutput &&) = delete;

	// --- Unused by this platform
	stiHResult CallbackRegister (PstiObjectCallback, size_t, size_t) { return stiRESULT_SUCCESS; }
	stiHResult CallbackUnregister (PstiObjectCallback, size_t) { return stiRESULT_SUCCESS; }
	virtual void DisplaySettingsGet (SstiDisplaySettings *) const {}
	virtual stiHResult DisplaySettingsSet (const SstiDisplaySettings *) { return stiRESULT_SUCCESS; }
	virtual int GetDeviceFD () const { return -1; }

	stiHResult Initialize () { return stiRESULT_SUCCESS; }
	stiHResult Startup () { return stiRESULT_SUCCESS; }
	void Uninitialize () {}
	void Shutdown () {}
	void WaitForShutdown () {}
	// ----------

	stiHResult RemoteViewPrivacySet (EstiBool bPrivacy);
	stiHResult RemoteViewHoldSet (EstiBool bHold);

	stiHResult VideoPlaybackStart ();
	stiHResult VideoPlaybackStop ();

	stiHResult VideoPlaybackFrameGet (IstiVideoPlaybackFrame **videoFrameOut);
	stiHResult VideoPlaybackFramePut (IstiVideoPlaybackFrame *videoFrame);
	stiHResult VideoPlaybackFrameDiscard (IstiVideoPlaybackFrame *videoFrame);
    
	stiHResult VideoPlaybackSizeGet (uint32_t *width, uint32_t *height) const;
	stiHResult VideoPlaybackCodecSet (EstiVideoCodec eVideoCodec);
	stiHResult VideoCodecsGet (std::list<EstiVideoCodec> *codecs);
    
	stiHResult CodecCapabilitiesGet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);
	stiHResult CodecCapabilitiesSet (
		EstiVideoCodec eCodec,
		SstiVideoCapabilities *pstCaps);

	void PreferredDisplaySizeGet (
		unsigned int *displayWidth,
		unsigned int *displayHeight) const;

protected:
	stiHResult KeyframeRequest (bool forceNow = false);
	stiHResult VideoPlaybackBlackFramePut ();
	stiHResult FormatDescriptionUpdate (IstiVideoPlaybackFrame *frame);

	stiHResult VideoDecoderCreate ();
	stiHResult VideoDecoderInvalidate ();
	stiHResult VideoFrameDecoded (CMSampleBufferRef sampleBuffer) const;

private:
	bool m_playing = false;
	bool m_privacy = false;
	bool m_hold = false;
	bool m_awaitingKeyframe = true;

	CstiTimer m_keyframeRequestTimer;
	std::chrono::steady_clock::time_point m_lastKeyframeRequestTime;

	size_t m_framesCreated = 0;
	std::queue<IstiVideoPlaybackFrame *> m_emptyFrames;
	std::recursive_mutex m_framesMutex;

	EstiVideoCodec m_codec = estiVIDEO_NONE;
	CMVideoFormatDescriptionRef m_formatDescription = nullptr;
	VTDecompressionSessionRef m_decoder = nullptr;
	bool m_decoderIsInvalid = false;
	size_t m_consecutiveDecoderErrorCount = 0;

	std::map<EstiVideoCodec, SstiVideoCapabilities*> m_codecCapabilities;
	void *m_preferredDisplaySizeChangedToken = nullptr;
};


} // namespace vpe
