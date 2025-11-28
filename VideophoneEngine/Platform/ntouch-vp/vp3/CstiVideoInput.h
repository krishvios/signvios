// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015-2020 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "CstiVideoInputBase2.h"

struct tofFocusPosition
{
	int distance;
	int position;
};

class CstiVideoInput : public CstiVideoInputBase2
{
public:
	CstiVideoInput () = default;
	~CstiVideoInput () override = default;

	CstiVideoInput (const CstiVideoInput &other) = delete;
	CstiVideoInput (CstiVideoInput &&other) = delete;
	CstiVideoInput &operator= (const CstiVideoInput &other) = delete;
	CstiVideoInput &operator= (CstiVideoInput &&other) = delete;

	static const std::vector<tofFocusPosition> m_tofFocusVector;

	stiHResult SingleFocus () override;  // async

private:

	void eventSingleFocus (int distance);

};
