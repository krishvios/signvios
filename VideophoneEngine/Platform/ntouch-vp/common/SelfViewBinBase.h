#pragma once

#include "GStreamerBin.h"
#include "stiDefs.h"

class SelfViewBinBase : public GStreamerBin
{
public:
	SelfViewBinBase();
	~SelfViewBinBase () override = default;

	SelfViewBinBase (const SelfViewBinBase &other) = delete;
	SelfViewBinBase (SelfViewBinBase &&other) = delete;
	SelfViewBinBase &operator= (const SelfViewBinBase &other) = delete;
	SelfViewBinBase &operator= (SelfViewBinBase &&other) = delete;

	void create(void *qquickItem);

private:
};
