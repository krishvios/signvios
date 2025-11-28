#pragma once

#include "EncodePipelineBase2.h"
#include "CaptureBin.h"
#include "EncodeBin.h"
#include "SelfViewBin.h"

class EncodePipeline : public EncodePipelineBase2
{
public:

	EncodePipeline () = default;
	~EncodePipeline () override = default;

	EncodePipeline (const EncodePipeline &other) = delete;
	EncodePipeline (EncodePipeline &&other) = delete;
	EncodePipeline &operator= (const EncodePipeline &other) = delete;
	EncodePipeline &operator= (EncodePipeline &&other) = delete;

protected:

	const CaptureBinBase2 &captureBinBase2Get () const override
	{
		return m_captureBin;
	}

	CaptureBinBase2 &captureBinBase2Get () override
	{
		return m_captureBin;
	}

private:

	void captureBinCreate () override;
	std::unique_ptr<EncodeBinBase> encodeBinCreate (const CaptureBinBase2 &captureBin) override;
	std::unique_ptr<SelfViewBinBase> selfViewBinCreate (void *qquickItem) override;

	CaptureBin m_captureBin;
};
