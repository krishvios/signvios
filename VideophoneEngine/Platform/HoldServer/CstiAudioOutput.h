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
	
	stiHResult Initialize ();
	
	virtual stiHResult AudioPlaybackStart ();

	virtual stiHResult AudioPlaybackStop ();

	virtual stiHResult AudioPlaybackCodecSet (
		EstiAudioCodec eAudioCodec);

	virtual stiHResult AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame);

	virtual int GetDeviceFD ();

	virtual void DtmfReceived (
		EstiDTMFDigit eDtmfDigit);
	
	virtual stiHResult CallbackRegister (
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam,
		size_t CallbackFromId);

	virtual stiHResult CallbackUnregister (
		PstiObjectCallback pfnVPCallback,
		size_t CallbackParam);
};

using CstiAudioOutputSharedPtr = std::shared_ptr<CstiAudioOutput>;

#endif //#ifndef CSTIAUDIOOUTPUT_H
