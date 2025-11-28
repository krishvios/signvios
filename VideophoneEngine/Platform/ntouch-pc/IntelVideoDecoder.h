#pragma once
#include "IVideoDecoder.h"
#include <map>

#if D3D_SURFACES_SUPPORT
#pragma warning(disable : 4201)
#include <d3d9.h>
#include <dxva2api.h>
#endif

#include <queue>
#include <memory>
#include <mutex>

#include "mfxcommon.h"
#include "mfxvideo.h"
#include "mfxvideo++.h"
#include "IntelWorkerFrame.h"

class IntelVideoDecoder :
	public IVideoDecoder
{
public:
	IntelVideoDecoder ();
	~IntelVideoDecoder ();
	bool decode (IstiVideoPlaybackFrame *frame, uint8_t *flags) override;
	std::shared_ptr<IVideoDisplayFrame> frameGet () override;
	void frameReturn (std::shared_ptr<IVideoDisplayFrame>) override;
	bool codecSet (EstiVideoCodec codec) override;
	void clear () override;
	std::string decoderDescription () override;
	static vpe::DeviceSupport supportGet (EstiVideoCodec codec, uint16_t profile, uint16_t level);

private:
	const int StartingWorkerFrameCount = 5;
	int outputFrameCount = StartingWorkerFrameCount;

	std::recursive_mutex m_outputFramesLock;
	std::recursive_mutex m_workFramesLock;

	MFXVideoSession m_mfxSession;
	std::unique_ptr<MFXVideoDECODE> m_pmfxDEC;
	mfxVideoParam m_videoParameters{ 0 };
	mfxDecodeStat m_stats{ 0 };

	std::queue<std::shared_ptr<IntelWorkerFrame>> m_outputFrames;

	mfxU32 m_currentCodecId{ 0 };
	bool m_decoderInitialized = false;
	int m_workerFrameSuggestedNumber = StartingWorkerFrameCount;
	std::queue<std::shared_ptr<IntelWorkerFrame>> m_workFrames;

	static MFXVideoSession& getSession ();
};

