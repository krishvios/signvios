#ifndef CSTIAUDIOOUTPUT_H
#define CSTIAUDIOOUTPUT_H

// Includes
//

#include <sstream>
#include <memory>

#include "stiSVX.h"
#include "stiError.h"
#include "stiOSMutex.h"
#include "BaseAudioOutput.h"
#include "BaseAudioInput.h"

#include "TPCircularBuffer.h"

// iOS audio support
#include <AudioToolbox/AudioToolbox.h>


//
// Constants
//

const int AUDIO_LATENCY_MILLIS = 125; // about 1/8 of a second
const int AUDIO_LATENCY_PACKETS = 8000*AUDIO_LATENCY_MILLIS/1000;
//
// Typedefs
//
typedef struct {
	int16_t *mData;
	uint32_t mCount; // number of packets
	uint32_t mCapacity; // -> not currently being used -> perhaps this whole struct is unecessary.
} SstiAudioBuffer;

//
// Forward Declarations
//


//
// Globals
//

//
// Class Declaration
//
class CstiAudio : public BaseAudioOutput, public BaseAudioInput
{

// audio library callbacks
// really they're class members but they're called from the OS layer
friend void CstiAudioSessionInterruptCallback( void* , UInt32 );
friend OSStatus CstiAudioOuputBusCallback(void              *inRefCon,
																 AudioUnitRenderActionFlags *ioActionFlags,
																 const AudioTimeStamp       *inTimeStamp,
																 UInt32                      inBusNumber,
																 UInt32                      inNumberFrames,
																 AudioBufferList            *ioData);
friend OSStatus CstiAudioInputBusCallback(void                       *inRefCon,
																AudioUnitRenderActionFlags *ioActionFlags,
																const AudioTimeStamp       *inTimeStamp,
																UInt32                      inBusNumber,
																UInt32                      inNumberFrames,
																AudioBufferList            *ioData);

friend OSStatus CstiAudioInputPCM2uLaw(AudioConverterRef             inAudioConverter,
									  UInt32                        *ioNumberDataPackets,
									  AudioBufferList               *ioData,
									  AudioStreamPacketDescription  **outDataPacketDescription,
									  void                          *inUserData);
friend OSStatus CstiAudioInputuLaw2PCM(AudioConverterRef             inAudioConverter,
									  UInt32                        *ioNumberDataPackets,
									  AudioBufferList               *ioData,
									  AudioStreamPacketDescription  **outDataPacketDescription,
									  void                          *inUserData);

private:	

public:
	CstiAudio ();
	
	stiHResult Initialize ();
	stiHResult Uninitialize();

// playback methods
	virtual stiHResult AudioPlaybackStart ();
	virtual stiHResult AudioPlaybackStop ();
	virtual stiHResult AudioPlaybackCodecSet (
		EstiAudioCodec eAudioCodec);
	virtual stiHResult AudioPlaybackPacketPut (
		SsmdAudioFrame * pAudioFrame) ;
	virtual stiHResult AudioOut (
		EstiSwitch eSwitch);

	virtual void DtmfReceived (EstiDTMFDigit eDtmfDigit);

	virtual int GetDeviceFD ();
	
	bool AudioEnabledGet() { return m_playbackEnabled || m_recordEnabled; }
	
// record methods

	stiHResult AudioRecordStart ();
	stiHResult AudioRecordStop ();
	stiHResult AudioRecordPacketGet(
		SsmdAudioFrame * pAudioFrame);
	stiHResult AudioRecordCodecSet (
		EstiAudioCodec eAudioCodec);
	stiHResult AudioRecordSettingsSet (
		const SstiAudioRecordSettings * pAudioRecordSettings);
	stiHResult EchoModeSet (
		EsmdEchoMode eEchoMode);
	stiHResult PrivacySet (
		EstiBool bEnable);
	stiHResult PrivacyGet (
		EstiBool *pbEnabled) const ;

	int GetDeviceFD () const;
	
	//IstiAudioInput.h
	virtual stiHResult CodecsGet (
		std::list<EstiAudioCodec> *pCodecs,
		EstiAudioCodec ePreferredCodec);

	//IstiAudioOutput.h
	virtual stiHResult CodecsGet (std::list<EstiAudioCodec> *pCodecs);
	
	void Reinit();
	
private:
	stiMUTEX_ID m_audioSessionMutex;
	stiMUTEX_ID m_playbackMutex;
	stiMUTEX_ID m_recordMutex;
	
	bool m_audioOpened;
	bool m_playbackEnabled;
	bool m_recordEnabled;
	bool m_recordPrivacy;
	bool m_bAudioOutEnabled;
		

	TPCircularBuffer m_playbackBuffer;
	TPCircularBuffer m_recordBuffer;
	
	//SsmdAudioFrame *AudioPlaybackPacketGet();
	
// Mac audio
	AudioDeviceID mInputDevID, mOutputDevID;
	AudioComponent mAudioComponent;
	AudioUnit mAudioUnit;
	AudioStreamBasicDescription mStreamFormat, muLawFormat;
	
	AudioConverterRef mPCM2uLaw;
	AudioConverterRef muLaw2PCM;
	
	
	AudioBufferList mAudioBuffer; // recorded pcm data
	SstiAudioBuffer mPCM2uLawBuffer; // data for storing pcm2uLaw
 	uint8_t *mPCM2uLawBufferPtr;
	
	AudioDeviceID FindAudioDeviceByUUID(const char* uuid);
	stiHResult OpenAudio();
	void CloseAudio();
	stiHResult CreatePCM2uLaw(AudioStreamBasicDescription *sourceDesc);
	stiHResult CreateuLaw2PCM(AudioStreamBasicDescription *sourceDesc);

	stiHResult CreateAudioUnit();
	
};

using CstiAudioSharedPtr = std::shared_ptr<CstiAudio>;

#endif //#ifndef CSTIAUDIOOUTPUT_H
