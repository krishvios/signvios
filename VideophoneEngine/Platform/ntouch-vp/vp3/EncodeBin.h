#pragma once

#include <gst/gst.h>
#include "stiDefs.h"
#include "stiSVX.h"
#include "GStreamerAppSink.h"
#include "EncodeBinBase.h"


class EncodeBin : public EncodeBinBase
{
public:

	EncodeBin () = default;
	~EncodeBin () override = default;

	EncodeBin (const EncodeBin &other) = delete;
	EncodeBin (EncodeBin &&other) = delete;
	EncodeBin &operator= (const EncodeBin &other) = delete;
	EncodeBin &operator= (EncodeBin &&other) = delete;

	EstiVideoFrameFormat bufferFormatGet () override;

protected:

	GStreamerElement videoConvertElementGet () override;
	GStreamerCaps encodeLegInputCapsGet () override;
	GStreamerCaps inputCapsGet() override;
	GStreamerCaps outputCapsGet () override;
	GStreamerElement encoderGet () override;

private:
	
	static constexpr const int MAX_SELFVIEW_DISPLAY_WIDTH = 1920;
	static constexpr const int MAX_SELFVIEW_DISPLAY_HEIGHT = 1080;
	
	// Methods
	const char* mediaTypeGet();
};
