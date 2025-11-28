#pragma once
#include "IstiVideoDecoder.h"
#include "IVideoDisplayFrame.h"

namespace vpe
{
	enum class DeviceSupport
	{
		Unavailable,
		Software,
		Hardware
	};
}

class IVideoDecoder
{
public:
	virtual ~IVideoDecoder () {};
	virtual bool decode (IstiVideoPlaybackFrame *frame, uint8_t *flags) = 0;
	virtual std::shared_ptr<IVideoDisplayFrame> frameGet() = 0;
	virtual void frameReturn (std::shared_ptr<IVideoDisplayFrame>) = 0;
	virtual bool codecSet (EstiVideoCodec codec) = 0;
	virtual void clear () = 0;
	virtual std::string decoderDescription () = 0;
};
