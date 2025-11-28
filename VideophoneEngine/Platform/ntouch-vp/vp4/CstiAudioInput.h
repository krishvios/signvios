// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIAUDIOINPUT_H
#define CSTIAUDIOINPUT_H

#include "CstiAudioInputBase.h"

class CstiAudioInput : public CstiAudioInputBase
{
public:
	
	CstiAudioInput () = default;
	
	~CstiAudioInput () override = default;
	
private:

};

using CstiAudioInputSharedPtr = std::shared_ptr<CstiAudioInput> ;

#endif // CSTIAUDIOINPUT_H
