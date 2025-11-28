// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
#pragma once

#include "ISLICRinger.h"


class CstiSLICRinger : public ISLICRinger
{
public:

	CstiSLICRinger () = default;
	~CstiSLICRinger () override = default;

	CstiSLICRinger (const CstiSLICRinger &other) = delete;
	CstiSLICRinger (CstiSLICRinger &&other) = delete;
	CstiSLICRinger &operator= (const CstiSLICRinger &other) = delete;
	CstiSLICRinger &operator= (CstiSLICRinger &&other) = delete;


	stiHResult startup ();

	stiHResult start() override;	 

	stiHResult stop() override;

	void deviceDetect () override;	 

private:
};
