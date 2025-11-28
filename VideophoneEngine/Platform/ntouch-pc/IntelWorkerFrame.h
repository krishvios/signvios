#pragma once
#include <vector>
#include "mfxcommon.h"
#include "mfxvideo.h"
#include "IVideoDisplayFrame.h"

class IntelWorkerFrame : public IVideoDisplayFrame
{
public:
	static const int MaxFrameWidth = 1920;
	static const int MaxFrameHeight = 1080;
	static const int BitsPerPixel = 12;// Y = 8, U=2, V=2 (I420 / NV12)
	static const int FrameSize = (MaxFrameWidth * MaxFrameHeight * BitsPerPixel) / 8;

	std::vector<mfxU8> m_surfaceBuffer;
	mfxFrameSurface1* m_frame;
	mfxSyncPoint m_syncPoint;
	IntelWorkerFrame ();
	~IntelWorkerFrame ();

	// Inherited via IVideoDisplayFrame
	virtual std::tuple<uint8_t *, uint8_t *, uint8_t *> data () override;
	virtual std::tuple<int, int, int> stride () override;
	virtual int widthGet () const override;
	virtual int heightGet () const override;
	virtual EstiColorSpace colorspaceGet () override;
};