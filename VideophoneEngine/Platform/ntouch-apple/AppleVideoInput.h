/*!
 * \file AppleVideoInput.h
 * \brief See AppleVideoInput.cpp
 *
 * Sorenson Communications Inc. Confidential. --  Do not distribute
 * Copyright 2017 Sorenson Communications, Inc. -- All rights reserved
 */

#pragma once

#include "BaseVideoInput.h"
#include <CoreMedia/CoreMedia.h>
#include <queue>
#include <mutex>

namespace vpe
{

struct ParameterSet;

class AppleVideoInput : public BaseVideoInput
{
public:
	AppleVideoInput ();
	virtual ~AppleVideoInput ();

	AppleVideoInput (const AppleVideoInput &) = delete;
	AppleVideoInput (AppleVideoInput &&) = delete;
	AppleVideoInput &operator= (const AppleVideoInput &) = delete;
	AppleVideoInput &operator= (AppleVideoInput &&) = delete;

	stiHResult VideoRecordStart () override;
	stiHResult VideoRecordStop () override;
	stiHResult VideoRecordSettingsSet (const SstiVideoRecordSettings *settings) override;
	stiHResult VideoRecordFrameGet (SstiRecordVideoFrame *videoFrame) override;
	stiHResult PrivacySet (EstiBool bEnable) override;
	stiHResult PrivacyGet (EstiBool *pbEnabled) const override;
	stiHResult KeyFrameRequest () override;
	stiHResult VideoCodecsGet (std::list<EstiVideoCodec> *codecs) override;
	stiHResult CodecCapabilitiesGet (EstiVideoCodec codec, SstiVideoCapabilities *caps) override;

	void DropFrame ();

private:

	void ClearFrameQueue ();

	// Alternatively there's a CMBufferQueue that might be useful...
	std::queue<CMSampleBufferRef> m_frames;
	std::mutex m_framesLock;

	EstiVideoCodec m_codec;
	EstiVideoProfile m_profile;
	unsigned int m_level;

	void *m_privacyObserver;
};


} // namespace vpe

