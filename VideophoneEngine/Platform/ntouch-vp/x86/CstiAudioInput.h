// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIAUDIOINPUT_H
#define CSTIAUDIOINPUT_H

#include "stiSVX.h"
#include "stiError.h"
#include "CstiAudioInputBase.h"


class CstiAudioInput : public CstiAudioInputBase
{
public:
	
	CstiAudioInput () = default;
	
	CstiAudioInput (const CstiAudioInput &other) = delete;
	CstiAudioInput (CstiAudioInput &&other) = delete;
	CstiAudioInput &operator= (const CstiAudioInput &other) = delete;
	CstiAudioInput &operator= (CstiAudioInput &&other) = delete;

	~CstiAudioInput () override = default;
	
private:
	void eventAudioPrivacySet (EstiBool enable);
	
};

using CstiAudioInputSharedPtr = std::shared_ptr<CstiAudioInput>;

#endif // CSTIAUDIOINPUT_H

