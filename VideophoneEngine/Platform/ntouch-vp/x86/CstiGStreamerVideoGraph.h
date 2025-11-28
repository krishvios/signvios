//Sorenson Communications Inc. Confidential. --  Do not distribute
//Copyright 2021 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiGStreamerVideoGraphBase2.h"
#include "CaptureBin.h"
#include "FrameCaptureBin.h"
#include "EncodePipeline.h"

class CstiGStreamerVideoGraph : public CstiGStreamerVideoGraphBase2
{
public:

	CstiGStreamerVideoGraph ();

	~CstiGStreamerVideoGraph () override = default;

	static float aspectRatioGet ();

protected:

	IstiVideoInput::SstiVideoRecordSettings defaultVideoRecordSettingsGet () override;

	std::unique_ptr<FrameCaptureBinBase> frameCaptureBinCreate () override;

	const EncodePipelineBase2 &encodeCapturePipelineBase2Get() const override
	{
		return m_encodeCapturePipeline;
	}

	EncodePipelineBase2 &encodeCapturePipelineBase2Get() override
	{
		return m_encodeCapturePipeline;
	}

private:

 	EncodePipeline m_encodeCapturePipeline;
};
