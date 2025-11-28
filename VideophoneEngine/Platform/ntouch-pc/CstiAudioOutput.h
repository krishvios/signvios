#ifndef CSTIAUDIOOUTPUT_H
#define CSTIAUDIOOUTPUT_H

// Includes
//
#include "stiSVX.h"
#include "stiError.h"

#include "IPlatformAudioOutput.h"
#include "BaseAudioOutput.h"
#include "CstiNativeAudioLink.h"
#include "FFAudioDecoder.h"
#include "CstiEventQueue.h"

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
class CstiAudioOutput : public CstiEventQueue, public BaseAudioOutput,  public IPlatformAudioOutput
{
public:
	CstiAudioOutput();

	stiHResult Initialize();
	stiHResult Uninitialize();
	stiHResult AudioPlaybackStart() override;
	stiHResult AudioPlaybackStop() override;
	stiHResult AudioPlaybackCodecSet(EstiAudioCodec eAudioCodec) override;
	stiHResult CodecsGet(std::list<EstiAudioCodec> *pCodecs) override;
	stiHResult AudioPlaybackPacketPut(SsmdAudioFrame * pAudioFrame) override;
	int GetDeviceFD() override;
	void DtmfReceived(EstiDTMFDigit eDtmfDigit) override;

	void outputSettingsSet (int channels, int sampleRate, int sampleFormat, int alignment) override;

private:
	static constexpr int g711ChannelCount = 1;
	static constexpr int g711SampleRate = 8000;

	void playbackStartEvent ();
	void playbackStopEvent ();
	void packetPutEvent (SsmdAudioFrame *pAudioFrame);
	void codecSetEvent (EstiAudioCodec audioCodec);
	void outputSettingsSetEvent (int channels, int sampleRate, int sampleFormat, int alignment);

	vpe::FFAudioDecoder m_audioDecoder;

	STI_OS_SIGNAL_TYPE m_fdPSignal = nullptr;

};

#endif //#ifndef CSTIAUDIOOUTPUT_H
