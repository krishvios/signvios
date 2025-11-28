// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2022 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIAUDIOOUTPUT_H
#define CSTIAUDIOOUTPUT_H

// Includes
//
#include "stiSVX.h"
#include "stiError.h"

#include "BaseAudioOutput.h"

#include <memory>

//
// Constants
//

//
// Typedefs
//

//
// Forward Declarations
//


//
// Globals
//

//
// Class Declaration
//
class CstiAudioOutput : public BaseAudioOutput
{
public:
	CstiAudioOutput ();
	
	virtual ~CstiAudioOutput ();

	stiHResult Initialize ();

	virtual stiHResult AudioPlaybackStart ();

	virtual stiHResult AudioPlaybackStop ();

	virtual stiHResult AudioPlaybackCodecSet (
		EstiAudioCodec eAudioCodec);

	virtual stiHResult AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame);
		
	virtual void DtmfReceived (EstiDTMFDigit eDtmfDigit);

	virtual int GetDeviceFD ();

};

using CstiAudioOutputSharedPtr = std::shared_ptr<CstiAudioOutput>;

#endif //#ifndef CSTIAUDIOOUTPUT_H
