#ifndef CSTIAUDIOOUTPUT_H
#define CSTIAUDIOOUTPUT_H

// Includes
//
#include "stiSVX.h"
#include "stiError.h"
#include "stiOSSignal.h"
#include "BaseAudioOutput.h"

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
	CstiAudioOutput (uint32_t nCallIndex);
	~CstiAudioOutput ();
	
	virtual stiHResult AudioPlaybackStart ();

	virtual stiHResult AudioPlaybackStop ();

	virtual stiHResult AudioPlaybackCodecSet (
		EstiAudioCodec eAudioCodec);

	virtual stiHResult AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame);

	virtual int GetDeviceFD ();

	virtual void DtmfReceived (
		EstiDTMFDigit eDtmfDigit);

	virtual stiHResult AudioOut (EstiSwitch eSwitch) { return stiRESULT_SUCCESS; }
	
private:
	uint32_t m_nCallIndex;
	STI_OS_SIGNAL_TYPE m_fdPSignal;
};

using CstiAudioOutputSharedPtr = std::shared_ptr<CstiAudioOutput>;

#endif //#ifndef CSTIAUDIOOUTPUT_H
