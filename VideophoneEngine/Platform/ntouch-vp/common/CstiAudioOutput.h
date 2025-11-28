// Sorenson Communications Inc. Confidential. --  Do not distribute
// Copyright 2015 Sorenson Communications, Inc. -- All rights reserved

#ifndef CSTIAUDIOOUTPUT_H
#define CSTIAUDIOOUTPUT_H

// Includes
//
#include "BaseAudioOutput.h"
#include "CstiAudioHandler.h"

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
	
	CstiAudioOutput (const CstiAudioOutput &other) = delete;
	CstiAudioOutput (CstiAudioOutput &&other) = delete;
	CstiAudioOutput &operator= (const CstiAudioOutput &other) = delete;
	CstiAudioOutput &operator= (CstiAudioOutput &&other) = delete;

	~CstiAudioOutput () override;
	
	stiHResult Initialize (
		CstiAudioHandler *pAudioHandler,
		EstiAudioCodec eAudioCodec);

	stiHResult AudioPlaybackStart () override;
	
	stiHResult AudioPlaybackStop () override;

	stiHResult AudioOut (
		EstiSwitch eSwitch) override;

#ifdef stiSIGNMAIL_AUDIO_ENABLED
	virtual stiHResult AudioPlaybackSettingsSet(
		SstiAudioDecodeSettings *pAudioDecodeSettings);
#endif
	stiHResult AudioPlaybackCodecSet (
		EstiAudioCodec eAudioCodec) override;

	stiHResult AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame) override;
		
	void DtmfReceived (
		EstiDTMFDigit eDtmfDigit) override;

	stiHResult CodecsGet (
		std::list<EstiAudioCodec> *pCodecs) override;

	int GetDeviceFD () override;

private:
	
	CstiAudioHandler *m_pAudioHandler;
	
	CstiSignalConnection m_audioHandlerReadyStateChangedSignalConnection;
};

using CstiAudioOutputSharedPtr = std::shared_ptr<CstiAudioOutput>;

#endif //#ifndef CSTIAUDIOOUTPUT_H
