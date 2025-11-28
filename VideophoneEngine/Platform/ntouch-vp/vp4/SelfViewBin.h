#pragma once

#include "SelfViewBinBase.h"
#include "stiDefs.h"

class SelfViewBin : public SelfViewBinBase
{
public:
	SelfViewBin() = default;
	~SelfViewBin () override = default;

	SelfViewBin (const SelfViewBin &other) = delete;
	SelfViewBin (SelfViewBin &&other) = delete;
	SelfViewBin &operator= (const SelfViewBin &other) = delete;
	SelfViewBin &operator= (SelfViewBin &&other) = delete;

private:
};
