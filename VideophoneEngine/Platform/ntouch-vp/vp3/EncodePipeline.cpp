#include "EncodePipeline.h"
#include "stiSVX.h"
#include "stiTools.h"

void EncodePipeline::captureBinCreate ()
{
	m_captureBin.create ();
}

std::unique_ptr<EncodeBinBase> EncodePipeline::encodeBinCreate (
	const CaptureBinBase2 &captureBin)
{
	auto bin = sci::make_unique<EncodeBin>();

	bin->create (captureBin);

	return bin;
}


std::unique_ptr<SelfViewBinBase> EncodePipeline::selfViewBinCreate (
	void *qquickItem)
{
	auto bin = sci::make_unique<SelfViewBin>();

	bin->create (qquickItem);

	return bin;
}

